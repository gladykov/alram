# alram
Alram - Alarm for RAM, when available memory is below threshold, with optional task killer.

TL;DR Free RAM monitor.

For Linux.

![alt text](https://github.com/gladykov/alram/blob/main/alram.png?raw=true)

## What?

It will show you warning when memory is below defined threshold.
If you define kill list - it will kill those processes, when above happens.

## Why?

I drive without SWAP file, because I can. Which is OK in 80% of cases. But sometimes I forget about it, and I saturate memory. And this is a problem. So better to kill non important processes, than to hard restart.

And because it is never too late to write first program in C.

## How?

1. Build yourself or install from AUR
2. `systemctl --user enable alram.service --now` or `alram`


## Config

Create `~/.config/alram.conf` with:
```
# If your free RAM will drop below this percent, I will show notification
OCCUPIED_RAM_THRESHOLD=7
# Check every seconds
POLL_FREQUENCY=2
# Uncomment, to kill processes in case of low memory situation. 
#PROCEESES_TO_KILL=["spotify","slack","webstorm"]
```

## Contributions

Are welcomed

## BTW.

I use Arch.
