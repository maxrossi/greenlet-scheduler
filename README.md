# carbon-scheduler
Provides channels and a scheduler for Greenlet coroutines. 
Tasklet and channel scheduling order and behaviour has been designed to match that of Stackless python as possible.
Much of the API has remained the same too, but only a portion of the full stackless API has been implemented. This is mainly due to the limited use that Eve Online makes of stackless python functionality.


## Scheduling a Tasklet

```
def foo(x):
  print(x)

import scheduler
# schedule a tasklet to be run
scheduler.tasklet(foo)("hello")
scheduler.tasklet(foo)("world!")

# run all scheduled tasklets
scheduler.run()

print("end!")
```

output
```
hello
world!
end!
```

Notice that tasklets run in the order that they are scheduled. When there are no more tasklets in the queue, run returns.

## Sending data between tasklets using channels

```
def sender(chan, x):
  print("channel sending")
  chan.send(x)
  print("sender finished")

def receiver(chan):
  print("receiver receiving ...")
  r = chan.receive()
  print("received ", r)

import scheduler
channel = scheduler.channel()
_tasklet_A = scheduler.tasklet(receiver)(channel)
_tasklet_B = scheduler.tasklet(sender)(channel, "Joe")

scheduler.run()
```
output
```
receiver receiving ...
channel sending
received Joe
```

Explanation:
When `_tasklet_A` calls `receive()` on the channel, it yields it's execution to the scheduler, since no tasklet is waiting to send on that channel. We say that `_tasklet_A` is now blocked on the channel.
When `_tasklet_B` calls `send()` on the channel, `_tasklet_A` gets 'unblocked' and continues execution, while `_tasklet_B` gets scheduled to run later.

### bug [PLAT-6083](https://ccpgames.atlassian.net/browse/PLAT-6083)
Notice that `sender finished` is not printed, this is becuase `_tasklet_B` has been re-scheduled to the back of the scheduler queue
An with an extra call to `scheduler.run()`, `_tasklet_B` runs, and we can see the output:

```
sender finished
```

This is a bug, all scheduled child tasklets ***should*** run before scheduler.run exits.

## Channel preference

Notice how in the above example, when the sender sent the value the program ran the receiving tasklet, and scheduled the sending tasklet to run later.
This is called channel preference. The default preference is to prefer the receiver.

There are three channel preference settings:
`-1` prefer receiver
`1` prefer sender
`0` prefer neither

Prefer sender will switch to the sending tasklet, while scheduling the receiver to run later
Prefer neither will switch back to the channel that unblocked the other, whether that be the sender or receiver:

So if in the above example, we set the channel preference to zero
```
channel = scheduler.channel()
channel.preference = 0
```

We keep the same tasklet order (with the sender unblocking the receiver)
```
scheduler.tasklet(receiver)(channel)
scheduler.tasklet(sender)(channel, "Joe")
scheduler.run()
```

will result in this out:

```
receiver receiving ...
channel sending
sender finished
```
with the receiver scheduled to run later

where as if we swap the tasklet order, and have the receiver unblock the sender:

```
scheduler.tasklet(sender)(channel, "Joe")
scheduler.tasklet(receiver)(channel)
scheduler.run()
```
The receiver will get run after the channel operation, with the sender scheduled to run later

```
channel sending
receiver receiving ...
received Joe
```

The difference between prefer neither, and either prefer receive or prefer sender, is that prefer receive and prefer sender do not rely on tasklet order
the receiver or the sender will be switched to immediately after the channel operation depending on the preference, regardless of which tasklet unblocked the other.
The behaviour of prefer neither depends on the order.
