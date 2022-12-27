# LapOS is 10 years old!

This year, 2022-12-11 marks the 10th anniversary of the initial commit of
LapOS! I take this time to reflect a bit on where this project started
from, where it stands now, and what are the possible plans for the
future.

## Origins

I experimented with 8086 assembly in my teens, and also did some
experiments with 32-bit protected mode. Next step would have been
enabling paging, but I didn't go into that at that time. It was in
2012 that I decided to have a go at the problem, and this time, I
also wanted to expand the scope of the project to include as much C as
possible. This resulted in a tiny kernel-like thing that initially did
nothing but display a spinning character on the screen. Later, it also
did nothing else, but achieved it by writing at another address that
got mapped to the video RAM by paging. With this, the original goal
was achieved.

After that, the project went dormant for long periods, and I worked on
it only when I had the time, energy and motivation -- and that
happened quite rarely. I worked on it in 2014, 2015 and 2018 (and one
minor commit in 2016).

## Present

In 2022, for some reason I took out LapOS again, and made a most
frightening observation: LapOS no longer boots on real hardware!
Actually, this had been the case for 4 years! This had to be solved,
of course, but finding the bug was not easy, it was subtle. When I was
finished with this, I noticed that there is some work in progress on
several branches. I merged these, and completed a very important step
towards making a minimally functioning kernel: interrupts. As of now,
IRQ 0, the interval timer and IRQ 1, the keyboard are working. Neither
does anything useful, but a stub ISR is in place.

## Future

Next step would be a meaningful scheduler that switches control
between two modules. Add address space separation to that, and you
have processes. From there, adding system calls means a working
microkernel.

At that point, there are important design decisions to make: what kind
of interface does the kernel expose to the processes? What kind of
interprocess communication is available? Adding filesystem and
networking would be to the kernelspace or userspace (that is, remain a
microkernel, or make it monolithic)?

It is not easy to do something as a hobby OS hacker that has not been done
better by others. However, I have a vision that is AFAIK unique: I
want to make the kernel essentially a terminal multiplexer or
window manager, and make every process able to display character
output, even networking and filesystem server processes (I am more
inclined towards a microkernel architecture). This could result in a
working OS that you can see in action on the screen: have an editor
and a shell open, while seeing the fs and network usage they generate
live on the respective servers' console output, which might make sense
from an academic or educational perspective.

I regard LapOS a personal project. I imagine myself 80 years old and
tinkering on IPv6 support. I see very little chance of achieving
something that many others will use, but I would definitely be
interested in how others find it useful. So, **if you are reading this,
and at least gave LapOS a try, please drop me an email at the email
address you see given at most recent commits.**

