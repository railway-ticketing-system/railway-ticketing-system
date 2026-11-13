"""Configure reviewable public Project views using the supported GraphQL API."""
import json,subprocess
from pathlib import Path
OWNER='railway-ticketing-system';NUMBER='1'
def gh(*args):return json.loads(subprocess.check_output(['gh',*args],text=True))
def gql(query,variables):
 r=subprocess.run(['gh','api','graphql','--input','-'],input=json.dumps({'query':query,'variables':variables}),text=True,capture_output=True,check=True);d=json.loads(r.stdout)
 if d.get('errors'):raise RuntimeError(d['errors'])
 return d['data']
def main():
 project=gh('project','view',NUMBER,'--owner',OWNER,'--format','json');pid=project['id']
 fields={x['name']:x['id']for x in gh('project','field-list',NUMBER,'--owner',OWNER,'--format','json')['fields']}
 views=gql('query($id:ID!){node(id:$id){... on ProjectV2{views(first:30){nodes{id name number layout}}}}}',{'id':pid})['node']['views']['nodes']
 needed=[('Original Jira sprints','TABLE_LAYOUT')]
 for name,layout in needed:
  if not any(v['name']==name for v in views):views.append(gql('mutation($p:ID!,$name:String!,$layout:ProjectV2ViewLayout!){createProjectV2View(input:{projectId:$p,name:$name,layout:$layout}){projectV2View{id name number layout}}}',{'p':pid,'name':name,'layout':layout})['createProjectV2View']['projectV2View'])
 for view in views:
  if view['name']=='Original Jira sprints':names=['Title','Assignees','Status','Original Jira sprint','Jira sprint start','Jira sprint finish','Story points retrospective','Related pull requests']
  elif view['layout']=='BOARD_LAYOUT':names=['Title','Assignees','Reviewers','Course sprint','Story points retrospective','Scheduled start','Scheduled finish','Original Jira sprint','Related pull requests']
  else:names=['Title','Assignees','Status','Course sprint','Story points retrospective','Scheduled start','Scheduled finish','Original Jira sprint','Jira sprint start','Jira sprint finish','Related pull requests','Implementation commit']
  filter_='milestone:"'+view['name']+'"'if view['name'].startswith('Course Sprint')else'is:issue'if view['name']=='Original Jira sprints'else''
  result=gql('mutation($v:ID!,$fields:[ID!]!,$filter:String!){updateProjectV2View(input:{viewId:$v,filter:$filter,configuration:{visibleFieldIds:$fields}}){projectV2View{id name number layout filter}}}',{'v':view['id'],'fields':[fields[n]for n in names if n in fields],'filter':filter_})['updateProjectV2View']['projectV2View']
  print(result['name'],result['layout'])
if __name__=='__main__':main()
