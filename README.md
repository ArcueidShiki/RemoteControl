# Version Control

```bash
1. git fetch origin [remote branch name]:[local branch name] # it won't merge to the current branch, unless ":" used to specify merge branch
2. ~git pull origin main # it will auto merge to the current branch.~
3. git rebase main --reapply-cherry-picks # resolve conflict to main. keep update ahead from main.
4. git add .; git commit -s
5. git push -f origin currentBranch
6. if directly using git rebase origin/main, you cannot create a pull request, it will automatically rebase to the remtoe reposiotry.
7. origin/main is the "local" ref to "remote" origin main. git pull origin/main will upadte from local branch named "origin/main"; git pull origin main, will fetch first, to ensure local origin/main updated.
```

# Configurations:

`charset=UNICODE`

## RemoteCtrl (server side)

**Starup without window(dialog)**

`Alt + Enter -> Configuration Properties -> Linker-> 1. Entry point: mainCRTStartup, 2. SubSystem: Windows.`

```cpp
#pragma comment(linker, "/subsystem:windwos /entry:WinMainCRTStartup")
#pragma comment(linker, "/subsystem:windwos /entry:mainCRTStartup")
#pragma comment(linker, "/subsystem:console /entry:WinMainCRTStartup")
#pragma comment(linker, "/subsystem:console /entry:mainCRTStartup")
```

### Memory Detection:

`https://kinddragon.github.io/vld/`

`Alt + Enter -> Configuration Properties -> C/C++ -> General -> Additional Include Directories -> vld (include) directory`

`default: C:\Program Files (x86)\Visual Leak Detector\include`

`Alt + Enter -> Configuration Properties -> Linker -> General -> Additional Library Directories -> vld (lib) directory`

`default: C:\Program Files (x86)\Visual Leak Detector\lib\Win64`

`Alt + Enter -> Configuration Properties -> Linker -> Debugging -> General Debug Info -> FULL`

### Permission:

**Run as Administrator**:

`cmd->secpol.msc->Local Policies->Security Options->`

1. Accounts: Administrator account status -> Enable
2. Accounts: Limit local account use of blank passwords to console logon only -> Disable

`win + x -> cmd(admin) -> net user Administrator *: set password to blank`

`Alt + Enter -> Configuration Properties -> Linker-> Manifest File -> UAC Execution Level -> requireAdministrator / asInvoker`

``Startup Directoru: cmd->shell:startup``

## Link Option:

``Alt + Enter -> Configuration Properties -> Advanced -> Use of MFC - > Use MFC in Static Library``

# Reflections:

`Design is much more important than functions.`

# TODO:

1. Add keyboard event
2. Replace terminate thread with atomic flag
3. Make window resizable
4. Minimize dialog hide it at task bar
5. Combine all the modules and components, integration test
6. Next introduce gtest framework.
7. CICD pipeline

# Summaries: