"""Reconstruct course milestones from existing commits; never invent Jira history."""
from pathlib import Path
import json,re,subprocess
ROOT=Path(__file__).resolve().parents[2]
# The numbers are new retrospective estimates, not a claim about original Jira story points.
GROUPS=[(9,12,'Course Sprint 1','2026-10-19','2026-10-23',3),(13,17,'Course Sprint 1','2026-10-19','2026-10-23',3),(18,24,'Course Sprint 1','2026-10-19','2026-10-23',5),(25,28,'Course Sprint 2','2026-10-26','2026-10-30',5),(29,40,'Course Sprint 3','2026-11-02','2026-11-06',5)]
REQS={9:'RTS-F-001',10:'RTS-F-002',11:'RTS-F-002',12:'RTS-F-001',13:'RTS-F-003',14:'RTS-F-004',15:'RTS-F-005',16:'RTS-F-004',17:'RTS-F-003',18:'RTS-F-006',19:'RTS-F-007',20:'RTS-F-008',21:'RTS-F-010',22:'RTS-F-009',23:'RTS-F-011',24:'RTS-F-012',25:'RTS-F-013',26:'RTS-F-014',27:'RTS-F-015',28:'RTS-F-016; RTS-SR-002',29:'RTS-NF-006; RTS-SR-002',30:'RTS-F-017',31:'RTS-F-017',32:'RTS-F-017',33:'RTS-F-018; RTS-NF-008',34:'RTS-NF-001',35:'RTS-NF-002',36:'RTS-NF-003',37:'RTS-NF-005; RTS-NF-006; RTS-SR-001; RTS-SR-002',38:'RTS-SR-003',39:'RTS-SR-004',40:'RTS-SR-005'}
def main():
 raw=subprocess.check_output(['git','log','--reverse','--format=%H%x09%aI%x09%an%x09%s'],cwd=ROOT,text=True)
 snapshot_path=ROOT/'docs/evidence/jira-snapshot-2026-10-13.json'
 original={i['key']:i for i in json.loads(snapshot_path.read_text())['issues']} if snapshot_path.exists() else {}
 items=[]
 for line in raw.splitlines():
  sha,date,author,title=line.split('\t',3);m=re.match(r'RTRS-(\d+):\s*(.*)',title)
  if not m:continue
  n=int(m[1]);group=next((g for g in GROUPS if g[0]<=n<=g[1]),None)
  if not group:continue
  prs=[3] if n==9 else [4] if n==10 else [5] if n==11 else [6,7] if n==12 else [8] if n<=24 else [9,10] if n<=28 else [11] if n<=32 else [12]
  items.append(dict(key=f'RTRS-{n}',title=m[2],requirements=REQS[n],assignee='deeptanshukumar',status='Done',course_sprint=group[2],planned_start=group[3],planned_end=group[4],story_points=group[5],estimate_basis='Retrospective estimate added for submission reconstruction; original Jira estimate not yet verified',commit=sha,actual_commit_date=date,commit_author=author,pull_requests=prs,jira_url=f'https://amarworkspace.atlassian.net/browse/RTRS-{n}',migration_basis='Repository commit and PR evidence; original Jira fields require live Jira/export verification'))
 for item in items:
  old=original.get(item['key'])
  if old:
   item['implementation_title']=item['title'];item['title']=old['summary'];item['jira_description']=old['description'];item['original_jira_sprints']=old['sprints_at_capture'];item['original_jira_status_at_capture']=old['status_at_capture'];item['original_jira_assignee_at_capture']=old['assignee_at_capture'];item['jira_snapshot_date']='2026-10-13';item['migration_basis']='Original Jira description/sprint membership from 13 Oct snapshot; delivered status/owner from later repository evidence'
 (ROOT/'docs/planning/backlog.json').write_text(json.dumps({'reconstructed_on':'2026-11-12','source':'Existing repository history plus recovered 13 Oct Jira snapshot; not current live Jira','items':items},indent=2)+'\n')
 s='# Backlog and Course Sprint Plan\n\nThis board reconstruction was prepared on 12 November 2026 from existing commits and merged PRs. Scheduled dates below are the course windows, not GitHub creation dates. Original Jira descriptions and six sprint memberships are preserved from a recovered 13 October snapshot; current live Jira dates/statuses remain unverified. No story-point values were returned for RTRS in that snapshot. Retrospective estimates are labelled explicitly and can be replaced after live Jira verification.\n\n'
 s+='| Milestone | Course window | Scope mapped from repository | Evidence |\n|---|---|---|---|\n| Dry run | 12–16 Oct | Authentication/profile completed before the formal sprint | PRs 3–7 |\n| Course Sprint 1 | 19–23 Oct | Existing auth foundation plus train search and booking | RTRS-9–24; PR 8 delivered 20 Oct |\n| Course Sprint 2 | 26–30 Oct | Cancellation, refund and restoration | RTRS-25–28; PRs 9–10 delivered 29 Oct |\n| Course Sprint 3 | 2–6 Nov | Admin, audit and security/NFR work | RTRS-29–40; PRs 11–12 delivered 3 Nov |\n\n'
 s+='Sprint 3 uses the explicitly permitted spillover window; the complete application was therefore not frozen at the end of Sprint 2. The verified final feature commit is `6b5464298c949806ebba0b7bd1d903c962b5255d` on 3 November; Amar merged it into main in PR 12. The current submission work updates documents/testing evidence without rewriting that history.\n\n'
 s+='| Ticket | Requirement | Story | Owner | Points* | Course milestone / original Jira sprint | Actual commit date | Evidence |\n|---|---|---|---|---:|---|---|---|\n'
 for x in items:s+=f"| [{x['key']}]({x['jira_url']}) | {x['requirements']} | {x['title']} | {x['assignee']} | {x['story_points']} | {x['course_sprint']} / {', '.join(z['name'] for z in x.get('original_jira_sprints',[]))} | {x['actual_commit_date'][:10]} | [commit](https://github.com/railway-ticketing-system/railway-ticketing-system/commit/{x['commit']}) / "+', '.join(f'[PR {n}](https://github.com/railway-ticketing-system/railway-ticketing-system/pull/{n})' for n in x['pull_requests'])+' |\n'
 s+='\n*Points are retrospective estimates, not imported Jira data. Course Sprint 1 includes foundation work already completed during the dry run. Current Github timestamps remain the true import time.\n\n## Remaining team actions\n\n| Owner | Action | Status |\n|---|---|---|\n| Amar | Upload both sprint videos and perform final human acceptance/demo | Pending external team action |\n| Aaryaprasad | Review requirements/deviations and record acceptance | Pending external team action |\n| Deeptanshu | Verify imported board against original Jira sprint dates and story points once connected | Awaiting Jira access |\n\n## Scope not fully represented by an original implementation ticket\n\nRTS-NF-004 recovery and RTS-NF-007 responsive UI receive new verification evidence in the System Validation and Browser Validation reports. They are not assigned invented historical Jira tickets.\n'
 (ROOT/'docs/planning/BACKLOG.md').write_text(s)
 print(f'Prepared {len(items)} evidence-linked backlog items')
if __name__=='__main__':main()
