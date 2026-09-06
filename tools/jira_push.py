# -*- coding: utf-8 -*-
"""Create the backlog in Jira over the REST API, from docs/jira_import.csv.

Jira's new UI routes CSV upload through Rovo, which *redrafts* the content
instead of importing it. That breaks traceability: the whole point is that a
story's title, points and labels match the requirement IDs in the SRS. This
script writes the rows literally.

    setx JIRA_SITE  pes1ug24am334.atlassian.net      (once, then reopen the shell)
    setx JIRA_EMAIL your-atlassian-login@example.com
    setx JIRA_TOKEN <token from id.atlassian.com/manage-profile/security/api-tokens>

    python tools/jira_push.py --project BMS --dry-run    # show what it would do
    python tools/jira_push.py --project BMS             # actually create

The token stays in your environment; this script only reads it.

Sprints are NOT set - assigning them needs the Agile board API and a sprint
that already exists. Every story is labelled sprint1..sprint4 instead, so you
can filter the backlog by label and drag them in one batch.
"""
import argparse
import base64
import csv
import json
import os
import sys
import urllib.error
import urllib.request

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CSV_PATH = os.path.join(ROOT, "docs", "jira_import.csv")


def die(msg):
    print("ERROR: " + msg, file=sys.stderr)
    sys.exit(1)


class Jira:
    def __init__(self, site, email, token):
        self.base = f"https://{site}/rest/api/3"
        cred = base64.b64encode(f"{email}:{token}".encode()).decode()
        self.headers = {
            "Authorization": f"Basic {cred}",
            "Content-Type": "application/json",
            "Accept": "application/json",
        }

    def call(self, method, path, body=None):
        url = path if path.startswith("http") else self.base + path
        data = json.dumps(body).encode() if body is not None else None
        req = urllib.request.Request(url, data=data, headers=self.headers, method=method)
        try:
            with urllib.request.urlopen(req) as r:
                raw = r.read().decode()
                return json.loads(raw) if raw else {}
        except urllib.error.HTTPError as e:
            detail = e.read().decode()[:600]
            raise RuntimeError(f"{method} {path} -> HTTP {e.code}\n{detail}") from None


def adf(text):
    """Plain text -> Atlassian Document Format. The v3 API will not take a string."""
    paras = [p for p in text.split("\n") if p.strip()]
    return {
        "type": "doc",
        "version": 1,
        "content": [{"type": "paragraph",
                     "content": [{"type": "text", "text": p}]} for p in paras],
    }


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--project", required=True, help="project key, e.g. BMS")
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()

    site = os.environ.get("JIRA_SITE")
    email = os.environ.get("JIRA_EMAIL")
    token = os.environ.get("JIRA_TOKEN")
    if not (site and email and token):
        die("set JIRA_SITE, JIRA_EMAIL and JIRA_TOKEN first (see the docstring)")

    rows = list(csv.DictReader(open(CSV_PATH, encoding="utf-8")))
    epics = [r for r in rows if r["Issue Type"] == "Epic"]
    stories = [r for r in rows if r["Issue Type"] == "Story"]
    print(f"csv: {len(epics)} epics, {len(stories)} stories, "
          f"{sum(int(s['Story point estimate']) for s in stories)} points")

    jira = Jira(site, email, token)

    # --- discover this project's issue type ids -----------------------------
    meta = jira.call("GET", f"/issue/createmeta?projectKeys={args.project}"
                            f"&expand=projects.issuetypes")
    projects = meta.get("projects") or []
    if not projects:
        die(f"project {args.project} not visible to {email} - wrong key, or no permission")
    types = {t["name"].lower(): t["id"] for t in projects[0]["issuetypes"]}
    print("issue types available:", ", ".join(sorted(types)))
    epic_t = types.get("epic")
    story_t = types.get("story") or types.get("task")
    if not (epic_t and story_t):
        die("could not find both an Epic and a Story issue type in this project")

    # --- discover the story points field ------------------------------------
    pts_field = None
    for f in jira.call("GET", "/field"):
        if f.get("name", "").lower() in ("story point estimate", "story points"):
            pts_field = f["id"]
            break
    print("story points field:", pts_field or "NOT FOUND (points will be skipped)")

    if args.dry_run:
        print("\n-- dry run, nothing created --")
        for e in epics:
            print(f"  EPIC  {e['Summary']}")
        for s in stories[:3]:
            print(f"  STORY {s['Summary']}  [{s['Priority']}, "
                  f"{s['Story point estimate']}pts, labels: {s['Labels']}]")
        print(f"  ... and {len(stories) - 3} more stories")
        return

    # --- create epics -------------------------------------------------------
    epic_key = {}
    for e in epics:
        fields = {
            "project": {"key": args.project},
            "issuetype": {"id": epic_t},
            "summary": e["Summary"],
            "description": adf(e["Description"]),
            "labels": e["Labels"].split(),
        }
        res = jira.call("POST", "/issue", {"fields": fields})
        epic_key[e["Summary"]] = res["key"]
        print(f"  {res['key']:<8} {e['Summary']}")

    # --- create stories, parented to their epic -----------------------------
    made = 0
    for s in stories:
        fields = {
            "project": {"key": args.project},
            "issuetype": {"id": story_t},
            "summary": s["Summary"],
            "description": adf(s["Description"]),
            "labels": s["Labels"].split(),
        }
        parent = epic_key.get(s["Epic Link"])
        if parent:
            fields["parent"] = {"key": parent}
        if pts_field and s["Story point estimate"]:
            fields[pts_field] = int(s["Story point estimate"])
        try:
            res = jira.call("POST", "/issue", {"fields": fields})
        except RuntimeError as err:
            # Priority schemes and points fields vary; retry without the
            # optional bits rather than losing the story.
            print(f"  retrying {s['Summary']} without optional fields\n    {err}")
            fields.pop(pts_field, None)
            res = jira.call("POST", "/issue", {"fields": fields})
        made += 1
        print(f"  {res['key']:<8} {s['Summary']}")

    print(f"\ndone: {len(epic_key)} epics, {made} stories in {args.project}")
    print("Sprints are not set - filter the backlog by label sprint1..sprint4 "
          "and drag them in.")


if __name__ == "__main__":
    main()
