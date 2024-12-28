# Workflow

1. git fetch origin [remote branch name]:[local branch name] # it won't merge to the current branch, unless ":" used to specify merge branch
2. ~git pull origin main # it will auto merge to the current branch.~
3. git rebase main --reapply-cherry-picks # resolve conflict to main. keep update ahead from main.
4. git add .; git commit -s
5. git push -f origin currentBranch
6. if directly using git rebase origin/main, you cannot create a pull request, it will automatically rebase to the remtoe reposiotry.
7. origin/main is the "local" ref to "remote" origin main. git pull origin/main will upadte from local branch named "origin/main"; git pull origin main, will fetch first, to ensure local origin/main updated.

# TODO 
1. Should be tested on vmware window >= 10
2. network communication should be decoupled with dlg UI.