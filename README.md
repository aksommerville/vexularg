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

## TODO

- [x] Make the universal menu disablable, and disable it. We can't control its use of color.
- [x] Map loader.
- [x] Sprite loader.
- [x] Hero sprite. Motion etc.
- [x] Camera lock.
- [x] Carry things.
- [x] Active things.
- [x] Hazards. ...NO. No danger, it's a friendly kind of game.
- - [x] What happens when you get hurt?
- [ ] Final maps.
- [ ] Final timing. First pick a duration based on the map. Must be difficult but possible to collect all the things. Then tune Moon Song to fit.
- - In the test map with a 2:00 limit, I can only get 7/8 items, and that's a stretch.
- [ ] Parallax background. Rain and clouds and such, that you see thru windows. Entirely decorative.
- [ ] Music.
- [ ] Sound effects. ...defer until the weekend, so I can have the MIDI rig on hand.
- [x] Session clock. Count down. "2 minutes to recite this incantation, collect the offering before that!"
- [x] Moon reciting the incantation.
- [ ] Hello modal.
- [ ] Gameover modal.
- [ ] Coyote time.
- [ ] Landing dust.
