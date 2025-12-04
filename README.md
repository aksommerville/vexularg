# Vexularg

Requires [Egg v2](https://github.com/aksommerville/egg2) to build.

B1T Jam, December 2025, theme "OFFERING".

Dot and Moon want to summon the dread ghoul Vexularg.
To do so, they'll need to gather a suitable offering.
Metroidvaniy fetch quest. Explore, find things, bring them back to the altar.
Time limit.

Pick things up with B, a la Sitter.
Include offerable items that also do things: trampoline, magnet, fan.
Maybe a TV to distract the murderous bat?
Make a murderous bat.

I'm thinking one giant map.

Don't generalize sprite groups like I did for Queen of Clocks, it's overkill.
Single list of live sprites, which we'll iterate the whole thing a lot. It's OK. Chill.

One tilesheet for terrain and one for sprites. Try not to need more.

Vertical jump is 4 meters normally, 6 with a balloon.

https://www.google.com/search?q=vexularg => 0 results! I guess we'll own it :)

## TODO

- [ ] Reconsider hazards. We'll have plenty of time for dev, and it could add some interesting play.
- [ ] Moving platforms?
- [ ] Parallax background. Rain and clouds and such, that you see thru windows. Entirely decorative.
- [ ] Sound effects. ...defer until the weekend, so I can have the MIDI rig on hand.
- [x] Coyote time.
- [x] Landing dust.
- [x] Rather than holding B in front of Moon, can we just do that automatically when we're in the finished state?
- - To that end, we could have thing track their quantized position and whether they're in the offeratorium. Then would could poll it fast and cheap.
- [ ] Scorekeeping. We could track Moon's false time and add that to the things score, to make speed count.
- [ ] Itch page.
- [ ] Revisit voicing of `2-wrath_of_vexularg`, can I make it crunchier?
- [x] Is it possible to lose the balloon? ...NO
- [x] Ground strike velocity is overestimated when dropping off a balloon.
- [x] What does it take to accidentally lose the collection after triggering acceleration?
- - It's possible. Likeliest way is if you release the balloon thru the ceiling hole. I'm not going to worry about it.

## Post-Jam

- [ ] Enable menu.
- [ ] Color graphics?

## Notes for the player

Include these in the Itch page.

- There are eight offerable things. All eight are required, to get the best ending.
- You don't need to be in the altar room when time runs out.
- If you're carrying a thing when time runs out, it's included in the offering, even if away from the altar.
- Things only need to be in the altar room; sitting on the altar itself is not necessary.
- The fan and the magnet repel or attract you while you hold them, if you pick them up by the business end.
- Fan and magnet will only operate on one thing at a time. Others behind that thing are not affected.
- Fan and magnet don't work through walls.
- It's not possible to lose the balloon. Though in some positions, it's very difficult to recover.
- The balloon will stop rising if you put something, or yourself, on top of it.
- No state is irreversible. No matter what you've done, if there's enough time, you can undo it.
