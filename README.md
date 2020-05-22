# Git Submodule

We are using git submodule as an internal dependency management. This allows branch selection stright from the parent repo as well as the ability to directly commiting to the sub repo. Together with platform's build system, we don't even have to config libary path.

To learn more: https://git-scm.com/book/en/v2/Git-Tools-Submodules

## Relevant Commands

```shell
git submodule update —remote —rebase
git submodule add <remote>
git checkout —recurse-submodule <submodule>
```

## Start Here

### Setup

First navigate to develop branch in eDNA-Server local repo

```shell
git config push.recurseSubmodules check
git config submodule.recurse true
git reset —hard
git pull
```

### Initializing Submodule

```shell
git submodule init
git submodule update
```

You should be able to compile at this point. If so stop here, else please let me know.

### Pulling Changes From Upstream

```shell
git submodule update —remote —rebase
```
