"""Read and verify the live GitHub Project against the local source backlog."""
from pathlib import Path
import collections,datetime,json,subprocess
ROOT=Path(__file__).resolve().parents[2];OWNER='railway-ticketing-system'
def gh(*args):return json.loads(subprocess.check_output(['gh',*args],cwd=ROOT,text=True))
def main():
 project=gh('project','view','1','--owner',OWNER,'--format','json');items=gh('project','item-list','1','--owner',OWNER,'--format','json','--limit','100')['items']
 meta=gh('api','graphql','-f','query={node(id:"'+project['id']+'"){... on ProjectV2{repositories(first:10){nodes{nameWithOwner}} views(first:30){nodes{id name number layout filter}}}}}')['data']['node']
 expected=json.loads((ROOT/'docs/planning/backlog.json').read_text())['items'];bytitle={i['title'].split(']')[0].lstrip('['):i for i in items if i['title'].startswith('[RTRS-')};checks={}
 checks['32 source stories present']=len(bytitle)==32
 checks['public project']=project['public']
 checks['repository linked']=OWNER+'/'+OWNER in [r['nameWithOwner']for r in meta['repositories']['nodes']]
 checks['no duplicate content URLs']=len({i['content']['url']for i in items})==len(items)
 checks['story status Done']=all(bytitle[x['key']].get('status')=='Done'for x in expected)
 checks['story assignees correct']=all(x['assignee']in bytitle[x['key']].get('assignees',[])for x in expected)
 checks['estimates match']=all(bytitle[x['key']].get('story points retrospective')==x['story_points']for x in expected)
 checks['course dates match']=all(bytitle[x['key']].get('scheduled start')==x['planned_start']and bytitle[x['key']].get('scheduled finish')==x['planned_end']for x in expected)
 checks['original Jira dates match']=all(bytitle[x['key']].get('jira sprint start')==x['original_jira_sprints'][-1]['startDate'][:10]and bytitle[x['key']].get('jira sprint finish')==x['original_jira_sprints'][-1]['endDate'][:10]for x in expected)
 checks['original sprint memberships match']=all(bytitle[x['key']].get('original Jira sprint')==', '.join(z['name']for z in x['original_jira_sprints'])for x in expected)
 checks['PR links match']=all(all('/pull/'+str(n)in bytitle[x['key']].get('related pull requests','')for n in x['pull_requests'])for x in expected)
 checks['commit links match']=all(x['commit']in bytitle[x['key']].get('implementation commit','')for x in expected)
 views={v['name']:v for v in meta['views']['nodes']}
 checks['delivery board exists']=views.get('Delivery board',{}).get('layout')=='BOARD_LAYOUT'
 checks['three course sprint boards exist']=all(views.get('Course Sprint '+str(n),{}).get('filter')=='milestone:"Course Sprint '+str(n)+'"'for n in range(1,4))
 checks['original Jira table exists']=views.get('Original Jira sprints',{}).get('layout')=='TABLE_LAYOUT'
 pr=gh('pr','view','45','--json','state')
 expected_status='In Progress' if pr['state']=='OPEN' else 'Done'
 checks['submission PR status matches live state']=any(i['content'].get('url','').endswith('/pull/45')and i.get('status')==expected_status for i in items)
 result={'checked_at':datetime.datetime.now(datetime.timezone.utc).isoformat(),'status':'PASS'if all(checks.values())else'FAIL','project_url':project['url'],'checks':checks,'total_items':len(items),'stories':len(bytitle),'course_counts':dict(collections.Counter(i.get('course sprint')for i in bytitle.values())),'views':meta['views']['nodes'],'note':'Date columns are original scheduled values; creation/activity timestamps remain actual import time.'}
 (ROOT/'docs/evidence/github-project-verification.json').write_text(json.dumps(result,indent=2)+'\n')
 print(result['status'],sum(checks.values()),'/',len(checks),'project checks;',len(items),'items')
 for k,v in checks.items():
  if not v:print('FAIL',k)
 if not all(checks.values()):raise SystemExit(1)
if __name__=='__main__':main()
