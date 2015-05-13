# nlfix
Some simple program to post on GitHub.

It was started as a elementary newline converter, as I had some problems with extisting solutions.

Worth saying that it's written with a BAD approach, and it is not an example of a practical coding. (E.g. many classes from the standrard library could be used.)

## How-To
Help output:
```
usage: [path] [extensions]
	OR [filename]
Available options are:
-mode         EOL mode (unix, dos, mac)
-skiperrors   skip any errors when processing files
-nolog        hide logs (can increase speed)
```

Sample use:
```
> nlfix.exe "C:\mydir" "txt;h;cpp" -mode unix
```

Recognised names: *tounix.exe*, *todos.exe*, *tomac.exe*

##Requirements
 - WINDOWS ONLY
 - The solution and project files included are for Visual Studio 2013 (you can use any compiler, though)
