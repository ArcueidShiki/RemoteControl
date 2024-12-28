# Workflow

1. git fetch origin main:main # it won't merge to the current branch. ":" used to specify merge branch
2. git pull origin main:main # it will auto merge to the current branch.
3. git rebase main # resolve conflict to main. keep update ahead from main.
4. git add .; git commit -s
5. git push -f origin currentBranch
6. if directly using git rebase origin/main, you cannot create a pull request, it will automatically rebase to the remtoe reposiotry.
7. origin/main is the "local" ref to "remote" origin main. git pull origin/main will upadte from local branch named "origin/main"; git pull origin main, will fetch first, to ensure local origin/main updated.
