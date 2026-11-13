"""Attach original dated Jira descriptions/sprint metadata to published backlog issues."""
from pathlib import Path
import json,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2];REPO='railway-ticketing-system/railway-ticketing-system'
def main():
 data=json.loads((ROOT/'docs/planning/backlog.json').read_text())['items'];published=json.loads((ROOT/'docs/evidence/github-backlog-import.json').read_text());urls={i['key']:i['url'] for i in published['issues']}
 for x in data:
  sprints=x.get('original_jira_sprints',[])
  snapshot_date=x.get('jira_snapshot_date','2026-10-13')
  body=f"## Original Jira record\n\nTicket: {x['jira_url']}\n\nThe following description, status and sprint metadata were read from Jira on **{snapshot_date}**. The earlier 13 October snapshot remains preserved separately in the repository; this import keeps the actual source dates.\n\n{x.get('jira_description','No description returned.')}\n\nOriginal sprint(s): "+', '.join(z['name'] for z in sprints)+'.\n'
  for s in sprints:
   if 'startDate'in s:body+=f"\n{s['name']} source dates: {s['startDate']} to {s.get('endDate','not returned')}.\n"
  body+=f"\nStatus then: {x.get('original_jira_status_at_capture','not returned')}; assignee then: {x.get('original_jira_assignee_at_capture') or 'unassigned'}. Those old values do not override later delivery evidence.\n\n## Existing implementation evidence\n\nRequirements: {x['requirements']}\n\nCommit: https://github.com/{REPO}/commit/{x['commit']}\nAuthor and actual date: {x['commit_author']} — {x['actual_commit_date']}\n\nRelated PRs:\n"+'\n'.join(f'- https://github.com/{REPO}/pull/{n}' for n in x['pull_requests'])+f"\n\n## Course milestone reconstruction\n\n{x['course_sprint']}: scheduled {x['planned_start']} to {x['planned_end']}. Course milestones consolidate the original six Jira sprints; they are separate fields, not rewritten original Jira membership.\n\nEstimate: **{x['story_points']} retrospective points**. The Jira estimate field was empty before reconciliation. These estimates were recorded in both Jira and GitHub on 13 November and labelled as submission estimates; they are not attributed to original sprint planning.\n\nThis GitHub issue was published on 12 November 2026 as a retrospective record of existing implementation, assigned to its recorded developer. Creation/closure/update timestamps remain actual GitHub activity times. Completion reflects existing code delivery, not a manufactured historical review or human acceptance. See docs/planning/BACKLOG.md and the dated sanitized Jira snapshot for provenance.\n"
  number=urls[x['key']].rsplit('/',1)[1]
  with tempfile.NamedTemporaryFile('w',suffix='.json') as f:
   json.dump({'title':f"[{x['key']}] {x['title']}",'body':body},f);f.flush()
   subprocess.run(['gh','api','-X','PATCH',f'repos/{REPO}/issues/{number}','--input',f.name],cwd=ROOT,check=True,stdout=subprocess.DEVNULL)
  print(x['key'],'original Jira description/sprint reconciled',flush=True)
 published['jira_snapshot_reconciled_on']='2026-11-12';published['original_jira_snapshot_date']='2026-10-13';published['live_jira_reconciled_on']='2026-11-13';published['basis']='Original Jira description/sprint membership from dated snapshot plus later Git/PR delivery evidence; live Jira read and reconciled 13 November'
 (ROOT/'docs/evidence/github-backlog-import.json').write_text(json.dumps(published,indent=2)+'\n')
if __name__=='__main__':main()
