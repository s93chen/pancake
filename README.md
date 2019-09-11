# pancake 🥞
A simple plain DAG resolver, like the most preliminary version of `make`.

### Usage
`pancake` takes a Pancakefile in below format (commands are tab-indented), builds and traverse the dependencies DAG in topological order, and executes the commands associated with each target. 
```
target1: dependency1 dependency2 ...
    command1
    command2
    ...
target2: dependency1 dependency2 ...
    command1
    command2
    ...
```

**To run:**
```
./pancake Pancakefile
```
