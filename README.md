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

### Channel Balance

The balance of a channel tells you how many tasklets are either waiting to receive or send on it.


A positive balance means that there are that many tasklets blocked on a send operation on that channel
```
def sender(chan, x):
  chan.send(x)

channel = scheduler.channel()

for i in range(10):
    scheduler.tasklet(sender)(channel, i)

print(channel.balance)

channel.receive()

print(channel.balance)
```
```
10
9
```

A Negative balance means that there are that many tasklets (balance * -1) tasklets blocked on a receive operation on that channel
```
def receiver(chan):
  chan.receive()

channel = scheduler.channel()

for i in range(10):
    scheduler.tasklet(receiver)(channel)

print(channel.balance)

channel.send()

print(channel.balance)
```
```
-10
-9
```

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


### Blocking the main tasklet

You are technically always running in a tasklet, even if you havent started one. That tasklet is called the `main` tasklet. If you imagine all tasklets organised into a tree of child and parent
tasklets, the main tasklet is the root node.

It is an exception to block the main tasklet indeffinitely:

```
import scheduler

# we are on the main tasklet
channel = scheduler.channel()

# call receive with nothing sending
channel.receive()
```
```
RuntimeError: Deadlock: the last runnable tasklet cannot be blocked.
```

If, however, there are other tasklets waiting in the run queue, scheduler will run those tasklets until the main tasklet is unblocked, before throwing the same exception:

```
def foo(x):
  print(x)

import scheduler

# we are on the main tasklet
scheduler.tasklet(foo)("1")
scheduler.tasklet(foo)("2")
scheduler.tasklet(foo)("3")

channel = scheduler.channel()
channel.receive()
```

The three scheduled tasklets will run before the exception is thrown. This is because any one of those tasklets may have caused main to become unblocked
```
1
2
3
Traceback (most recent call last): . . .
RuntimeError: Deadlock: the last runnable tasklet cannot be blocked.
```

In this situation, the scheduler will only run tasklets as long as main is blocked

```
def foo(x):
  print(x)

def unblock(chan):
  chan.send(1)

channel = scheduler.channel()

# we are on the main tasklet
scheduler.tasklet(foo)("1")
scheduler.tasklet(foo)("2")
scheduler.tasklet(unblock)(channel)
scheduler.tasklet(foo)("3")


r = channel.receive()

print("received ", r)

# now we run what is left of the tasklet queue
scheduler.run()
```
```
1
2
received  1
3
```

Notice that a call to `scheduler.run` is needed to run the last tasklet.

## Block trap

You can prevent any tasklet from yielding execution to another tasklet by setting `block_trap` to `True`:

```
import scheduler

def receiver(chan):
    print("about to call receive ...")
    chan.receive()

def sender(chan, x):
    channel.send(x)

channel = scheduler.channel()

unblockableTasklet = scheduler.tasklet(receiver)(channel)
scheduler.tasklet(sender)(channel, 1)

unblockableTasklet.block_trap = True

scheduler.run()
```
```
about to call receive ...
Traceback (most recent call last): ...

RuntimeError: Channel cannot block on a tasklet with block_trap set true
```

Notice that even though there are tasklets queued up scheduled to unblock it, this tasklet cannot block on the channel

Please note that even though stackless [documentation seems to suggest otherwise](https://stackless.readthedocs.io/en/3.7-slp/library/stackless/channels.html#channel.preference), channel preference does not respect `block_trap`, so neither does scheduler's channel preference implementation.

## Exceptoins

You can cause an exception to be raised on a running tasklet:

```
import _scheduler_debug as scheduler

class CustomError(Exception):
  pass

channel = scheduler.channel()

def foo(chan):
    try:
        print("blocking on send ...")
        # block on a send, we don't need this to complete
        chan.send(1)
    except CustomError as e:
        print("Exception Raised in foo: ", e)

s = scheduler.tasklet(foo)(channel)
s.run()

print("raising exception from main")
s.raise_exception(CustomError("This exception was raised from another tasklet"))

print("end")
```
```
blocking on send ...
raising exception from main
Exception Raised in foo:  This exception was raised from another tasklet
end
```

You can also send exceptions over channels:

```
class CustomError(Exception):
  pass

def sender(chan):
    print("sending exception over channel")
    chan.send_exception(CustomError, "this exception was sent over a channel")

def receiver(chan):
    try:
        chan.receive()
    except CustomError as e:
        print("caught CustomError exception on receiver: ", e)

channel = scheduler.channel()

scheduler.tasklet(sender)(channel)
scheduler.tasklet(receiver)(channel)
scheduler.run()
print("end")
```
```
sending exception over channel
caught CustomError exception on receiver:  this exception was sent over a channel
end
```

Stackless python's `send_throw` is also supported. You can re-throw a previously caught exception on a different tasklet by sending it over a channel

```
class CustomError(Exception):
  pass


def bar():
    raise CustomError("send throw example exception")

def f(testChannel):
    try:
        bar()
    except Exception:
        testChannel.send_throw(*sys.exc_info())

channel = scheduler.channel()
tasklet = scheduler.tasklet(f)(channel)

try:
    channel.receive()
except ValueError e:
    print("a thrown exception was received")
```
```
a thrown exception was received
end
```


### `TaskletExit`

Raising a TaskletExit exception on a tasklet causes that tasklet to be killed. A TaskletExit exception is contained within a Tasklet and does not propagate up.

```
def bar():
    print("about to raise")
    raise scheduler.TaskletExit()

t = scheduler.tasklet(bar)()
scheduler.run()

print(t.alive)
print("end")
```
```
about to raise
False
end
```

You can send a TaskletExit exception to a tasklet to kill that tasklet

```
def bar(chan):
    chan.receive()

channel = scheduler.channel()
t = scheduler.tasklet(bar)(channel)
scheduler.run()

print(channel.balance)
channel.send_exception(scheduler.TaskletExit, "exit!")
print(channel.balance)

print(t.alive)
print("end")
```
```
-1
0
False
end
```
### bug [PLAT-6099](https://ccpgames.atlassian.net/browse/PLAT-6099)
Currently `channel.send_exception(scheduler.TaskletExit,...` causes an exception to be rasied on the sending tasklet.

## Nesting tasklets & run order

Any tasklet can add another tasklet to the scheduler queue. Calling scheduler.run will ensure that all tasklets currently on the queue, plus any tasklets added to the queue by those tasklets, will be run. Tasklets will run in the order that they were added to the scheduler queue.

```
def log(s):
  print(s)

def bar():
  print("fourth")
  scheduler.tasklet(log)("fifth")

def foo():
  print("second")
  scheduler.tasklet(bar)()

scheduler.tasklet(log)("first")
scheduler.tasklet(foo)()
scheduler.tasklet(log)("third")
scheduler.run()
```
```
first
second
third
fourth
fifth
```
## Switching between tasklets directly with `switch`

You can switch directly to another tasklet, causing that tasklet to run immediately.

```
def log(s):
  print(s)

scheduler.tasklet(log)("added first")
scheduler.tasklet(log)("added second").switch()
scheduler.run()
```
```
added second
added first
```

## `bind`, `setup` & `insert`

You can create a tasklet without giving it a function to run

```
t = scheduler.tasklet()
print(t.alive)
```
```
False
```

In order to run, t must be bound to a function and given arguments

```
def foo(s):
    print(s)

t = scheduler.tasklet()
t.bind(foo, "I am running")
t.run()
```
```
I am running
```

Notice that we have to call t.run in order to run t. That is because t isn't yet in the scheduler queue. To insert t into the scheduler queue, we can either call `t.setup` or `t.insert`. With `t.setup`, you can provide args if not already provided in the call to bind.

setup example:
```
def foo(s):
    print(s)

t = scheduler.tasklet()
t.bind(foo)
t.setup("I am running")

scheduler.run()
```
```
I am running
```

insert example
```
def foo(s):
    print(s)

t = scheduler.tasklet()
t.bind(foo, ("I am running", ))
t.insert()

scheduler.run()
```
```
I am running
```

You can check whether a tasklet is in the scheduler queue by checking the `scheduled` member variable 

```
def foo(s):
    print(s)

t = scheduler.tasklet()
t.bind(foo, ("I am running", ))
print("scheduled: ", t.scheduled)
t.insert()
print("scheduled: ", t.scheduled)

scheduler.run()
```
```
scheduled:  False
scheduled:  True
I am running
```

Trying to run an unbound tasklet raises a RuntimeError
```
t = scheduler.tasklet()
t.run()
```
```
RuntimeError: Cannot run tasklet that is not alive (dead)
```

It is possible to un-bind a tasklet
```
def foo(s):
    print(s)

t = scheduler.tasklet()
t.bind(foo, ("I am running", ))

t.bind(None)
t.run()
```
```
RuntimeError: Cannot run tasklet that is not alive (dead)
```

## Threads

- There is one Tasklet scheduler queue per python thread.
- You **cannot** move tasklets between threads.
- Tasklets on different threads **cannot** switch to one another.
- You **can** send messages over channels between tasklets on different threads.

Sending a message to a tasklet on another channel causes that tasklet to be scheduled to run on that thread's scheduler queue.

```
import scheduler
import threading

channel = scheduler.channel()

def receiver(chan):
    r = chan.receive()
    print("received '{}' from different thread".format(r))

def otherThreadMainTasklet(chan):
    t = scheduler.tasklet(receiver)(chan)
    while(t.alive):
        scheduler.run()

recever_thread = threading.Thread(target=otherThreadMainTasklet, args=(channel,))
recever_thread.start()

channel.send("Hello from another thread!")
```
```
received 'Hello from another thread!' from different thread
```

notice how we call `scheduler.run` on the `recever_thread`. This is because **a tasklet on one thread cannot switch to a tasklet on another thread**. The receiving tasklet on `recever_thread` gets put in that thread's scheduler queue once it is unblocked by the send operation, but the python code is responsible for making sure that tasklet then runs.

The rules surrounding blocking on the Main tasklet apply per thread, since each thread has it's own main tasklet

```
def otherThreadMainTasklet():
    # we are on the main tasklet
    channel = scheduler.channel()

    # call receive with nothing sending
    channel.receive()

other_thread = threading.Thread(target=otherThreadMainTasklet, args=())
other_thread.start()
```
```
Exception in thread Thread-1 (otherThreadMainTasklet): . . .
RuntimeError: Deadlock: the last runnable tasklet cannot be blocked.
```
