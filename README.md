# cash
A simple Linux shell made for educational purposes. It uses the GNU readline library for input.

<p align="center">
  <img src=https://rimgo.pussthecat.org/sgDSCsx.png />
</p>

## Features
* A cute looking prompt.
* History, tab completion, and readline keybinds.
* Running processes in the background ('&').
* Pipes ('|').
* I/O redirection ('>', '>>', '<').
* String quotes (e.g: "hello").

## Running
```
git clone https://github.com/dimeritium-foil/cash
cd cash
make run
```

## Features I Might Add In The Future
* Common shortcuts like: ctrl + c [ kill the process ], ctrl + D [ exit ].
* Semicolons and conditional execution.
* An asynchronous SIGCHLD handler.
* ~~A more informative SIGCHLD handler, and a `jobs` command.~~ (TODO: Handle hypothetical corner cases.)
