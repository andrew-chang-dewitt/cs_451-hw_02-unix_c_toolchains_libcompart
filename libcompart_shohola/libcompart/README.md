By default, a static library is produced. For a dynamic library use:
```
LC_LIB=dynamic make
```
Then remember to set `LD_LIBRARY_PATH`.

By default, the debug version of the library is produced. For the release version use:
```
EXPERIMENT_PERFORM=YES make
```
