#!/usr/bin/env python3
"""Audit rewritten PKE teaching chapters against an immutable Git baseline."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import subprocess
import sys
from collections import Counter
from pathlib import Path
from typing import Iterable
from urllib.parse import unquote, urlsplit


REPO_ROOT = Path(__file__).resolve().parents[1]
DOC_ROOT = REPO_ROOT / "docs" / "pke-doc-master 2"
CHAPTERS = {
    1: "docs/pke-doc-master 2/chapter1_riscv.md",
    2: "docs/pke-doc-master 2/chapter2_installation.md",
    3: "docs/pke-doc-master 2/chapter3_traps.md",
    4: "docs/pke-doc-master 2/chapter4_memory.md",
    5: "docs/pke-doc-master 2/chapter5_process.md",
    6: "docs/pke-doc-master 2/chapter6_filesystem.md",
}

HEADING_RE = re.compile(r"^\s{0,3}#{1,6}\s+(.+?)\s*#*\s*$", re.MULTILINE)
REQUIRED_HEADING_RE = re.compile(
    r"(?:^第[一二三四五六]章|^\d+(?:\.\d+)+(?:\.?\s|$)|\bLab\b|challenge|bonus|挑战|实验|附加题)",
    re.IGNORECASE,
)
ANCHOR_RE = re.compile(
    r"<a\b[^>]*\b(?:id|name)\s*=\s*([\"'])(.*?)\1[^>]*>", re.IGNORECASE
)
MARKDOWN_TARGET_RE = re.compile(r"(!?)\[[^\]]*\]\(([^)]+)\)")
REFERENCE_TARGET_RE = re.compile(r"^\s*\[[^\]]+\]:\s*(\S+)", re.MULTILINE)
HTML_TARGET_RE = re.compile(
    r"<(?:img|a)\b[^>]*\b(?:src|href)\s*=\s*([\"'])(.*?)\1", re.IGNORECASE
)
URL_RE = re.compile(r"https?://[^\s<>\"']+")
FENCE_OPEN_RE = re.compile(r"^\s{0,3}(`{3,}|~{3,})(.*)$")


def normalize_text(text: str) -> str:
    return text.replace("\r\n", "\n").replace("\r", "\n")


def git_show(revision: str, path: str) -> str:
    result = subprocess.run(
        ["git", "show", f"{revision}:{path}"],
        cwd=REPO_ROOT,
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    if result.returncode != 0:
        message = result.stderr.decode("utf-8", errors="replace").strip()
        raise RuntimeError(f"cannot read {revision}:{path}: {message}")
    return normalize_text(result.stdout.decode("utf-8", errors="strict"))


def read_worktree(path: str) -> str:
    return normalize_text((REPO_ROOT / path).read_text(encoding="utf-8"))


def clean_heading(value: str) -> str:
    return re.sub(r"\s+", " ", value.strip())


def trim_outer_blank_lines(lines: list[str]) -> list[str]:
    start = 0
    end = len(lines)
    while start < end and not lines[start].strip():
        start += 1
    while end > start and not lines[end - 1].strip():
        end -= 1
    return lines[start:end]


def extract_fences(text: str) -> tuple[list[str], list[str]]:
    bodies: list[str] = []
    errors: list[str] = []
    active_char: str | None = None
    active_length = 0
    body: list[str] = []
    opening_line = 0

    for line_number, line in enumerate(text.split("\n"), start=1):
        if active_char is None:
            match = FENCE_OPEN_RE.match(line)
            if match:
                marker = match.group(1)
                active_char = marker[0]
                active_length = len(marker)
                opening_line = line_number
                body = []
            continue

        close_re = re.compile(
            rf"^\s{{0,3}}{re.escape(active_char)}{{{active_length},}}\s*$"
        )
        if close_re.match(line):
            bodies.append("\n".join(trim_outer_blank_lines(body)))
            active_char = None
            active_length = 0
            body = []
            opening_line = 0
        else:
            body.append(line)

    if active_char is not None:
        errors.append(f"unclosed fence opened at line {opening_line}")
    return bodies, errors


def clean_markdown_target(raw: str) -> str:
    target = raw.strip()
    if target.startswith("<"):
        closing = target.find(">")
        if closing >= 0:
            return target[1:closing].strip()
    # Markdown destinations may be followed by a quoted title.
    title_match = re.match(r"^(.*?)(?:\s+[\"'].*[\"'])\s*$", target)
    if title_match:
        target = title_match.group(1).strip()
    return target


def strip_url_punctuation(url: str) -> str:
    return url.rstrip('.,;:!?，。；：！？)]}')


def extract_evidence(text: str) -> dict[str, object]:
    headings = [clean_heading(item) for item in HEADING_RE.findall(text)]
    required_headings = [item for item in headings if REQUIRED_HEADING_RE.search(item)]
    anchors = [match[1].strip() for match in ANCHOR_RE.findall(text)]
    fences, fence_errors = extract_fences(text)

    images: list[str] = []
    links: list[str] = []
    for image_marker, raw_target in MARKDOWN_TARGET_RE.findall(text):
        target = clean_markdown_target(raw_target)
        if image_marker:
            images.append(target)
        else:
            links.append(target)
    links.extend(clean_markdown_target(item) for item in REFERENCE_TARGET_RE.findall(text))
    for match in HTML_TARGET_RE.finditer(text):
        target = match.group(2).strip()
        element = match.group(0).lstrip().lower()
        if element.startswith("<img"):
            images.append(target)
        else:
            links.append(target)

    # The source material contains one historical same-page link written as
    # ``(pagetablecook)`` rather than ``(#pagetablecook)``. Treat a bare target
    # that exactly matches an anchor in the same document as that anchor. This
    # preserves its meaning while allowing the rewrite to correct the syntax.
    anchor_names = set(anchors)
    links = [f"#{target}" if target in anchor_names else target for target in links]

    urls = [strip_url_punctuation(item) for item in URL_RE.findall(text)]
    fence_hashes = [hashlib.sha256(item.encode("utf-8")).hexdigest() for item in fences]

    return {
        "headings": headings,
        "required_headings": required_headings,
        "anchors": anchors,
        "fence_bodies": fences,
        "fence_hashes": fence_hashes,
        "images": images,
        "links": links,
        "urls": urls,
        "fence_errors": fence_errors,
    }


def missing_items(baseline: Iterable[str], current: Iterable[str]) -> list[str]:
    missing = Counter(baseline) - Counter(current)
    expanded: list[str] = []
    for value in sorted(missing):
        expanded.extend([value] * missing[value])
    return expanded


def fence_label(body: str) -> str:
    digest = hashlib.sha256(body.encode("utf-8")).hexdigest()[:12]
    first_line = next((line.strip() for line in body.split("\n") if line.strip()), "<empty>")
    if len(first_line) > 100:
        first_line = first_line[:97] + "..."
    return f"sha256:{digest} first-line:{first_line}"


def is_external_target(target: str) -> bool:
    parsed = urlsplit(target)
    return bool(parsed.scheme or parsed.netloc) or target.startswith("//")


def local_target_path(chapter_path: str, target: str) -> Path | None:
    value = unquote(target.strip())
    if not value or value.startswith("#") or is_external_target(value):
        return None
    value = value.split("#", 1)[0].split("?", 1)[0]
    if not value:
        return None
    candidate = (REPO_ROOT / chapter_path).parent / value
    if candidate.exists():
        return candidate
    doc_candidate = DOC_ROOT / value.lstrip("/")
    if doc_candidate.exists():
        return doc_candidate
    return candidate


def unresolved_local_targets(chapter_path: str, evidence: dict[str, object]) -> list[str]:
    unresolved: set[str] = set()
    targets = list(evidence["images"]) + list(evidence["links"])
    for target in targets:
        candidate = local_target_path(chapter_path, target)
        if candidate is not None and not candidate.exists():
            unresolved.add(target)
    return sorted(unresolved)


def counts(evidence: dict[str, object]) -> dict[str, int]:
    return {
        "headings": len(evidence["headings"]),
        "required_headings": len(evidence["required_headings"]),
        "anchors": len(evidence["anchors"]),
        "fenced_code_blocks": len(evidence["fence_bodies"]),
        "images": len(evidence["images"]),
        "links": len(evidence["links"]),
        "urls": len(evidence["urls"]),
    }


def verify_chapter(chapter: int, path: str, baseline_revision: str) -> dict[str, object]:
    baseline_text = git_show(baseline_revision, path)
    current_text = read_worktree(path)
    baseline = extract_evidence(baseline_text)
    current = extract_evidence(current_text)

    missing = {
        "required_headings": missing_items(
            baseline["required_headings"], current["required_headings"]
        ),
        "anchors": missing_items(baseline["anchors"], current["anchors"]),
        "fenced_code_blocks": [
            fence_label(body)
            for body in missing_items(baseline["fence_bodies"], current["fence_bodies"])
        ],
        "images": missing_items(baseline["images"], current["images"]),
        "links": missing_items(baseline["links"], current["links"]),
        "urls": missing_items(baseline["urls"], current["urls"]),
    }
    unresolved = unresolved_local_targets(path, current)
    errors = list(current["fence_errors"])
    if not current_text.strip():
        errors.append("chapter is empty")

    passed = not errors and not unresolved and all(not values for values in missing.values())
    return {
        "chapter": chapter,
        "path": path,
        "passed": passed,
        "baseline_counts": counts(baseline),
        "current_counts": counts(current),
        "missing": missing,
        "unresolved_local_targets": unresolved,
        "errors": errors,
    }


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline", default="c30acbe", help="baseline Git revision")
    parser.add_argument("--chapter", type=int, choices=sorted(CHAPTERS))
    parser.add_argument("--write-report", type=Path)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    selected = [args.chapter] if args.chapter else sorted(CHAPTERS)
    try:
        chapter_reports = [
            verify_chapter(chapter, CHAPTERS[chapter], args.baseline)
            for chapter in selected
        ]
    except (OSError, RuntimeError, UnicodeError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 2

    report = {
        "baseline": args.baseline,
        "passed": all(item["passed"] for item in chapter_reports),
        "chapters": chapter_reports,
    }
    rendered = json.dumps(report, ensure_ascii=False, indent=2, sort_keys=True) + "\n"
    if args.write_report:
        output_path = args.write_report
        if not output_path.is_absolute():
            output_path = REPO_ROOT / output_path
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_text(rendered, encoding="utf-8")

    for item in chapter_reports:
        state = "PASS" if item["passed"] else "FAIL"
        print(f"[{state}] Chapter {item['chapter']}: {item['path']}")
        for category, values in item["missing"].items():
            if values:
                print(f"  missing {category}: {len(values)}")
        if item["unresolved_local_targets"]:
            print(f"  unresolved local targets: {len(item['unresolved_local_targets'])}")
        for error in item["errors"]:
            print(f"  error: {error}")
    print("Overall:", "PASS" if report["passed"] else "FAIL")
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
