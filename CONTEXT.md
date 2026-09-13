# LED Game

This context defines the shared language for describing LED Game behavior. Specifications and design discussion use these terms consistently.

## Language

**Pulse**:
A white light launched from one end of the LED strip and moving toward the opposite end.
_Avoid_: Dot, projectile

**Target Zone**:
A contiguous yellow region of the LED strip that Pulses attempt to hit. Its brightness changes through a Breathing Cycle.
_Avoid_: Scoring Zone, Goal, Goal Zone

**Collision Span**:
The one or two adjacent LED positions occupied or crossed when opposing Pulses meet. Its overlap with the Target Zone determines the collision outcome.
_Avoid_: Collision Point, meeting area

**Hit**:
A collision whose Collision Span overlaps the Target Zone.
_Avoid_: Successful Collision, score, win

**Miss**:
A collision whose Collision Span does not overlap the Target Zone.
_Avoid_: Failed Collision, failure, death

**Progress**:
The accumulated challenge state represented by the Target Zone's configured size and Breathing Rate.
_Avoid_: Score, Difficulty, Level

**Breathing Cycle**:
The repeating brightness animation that makes the Target Zone fade between dim and bright. Its speed is the Breathing Rate.
_Avoid_: Zone Pulse, Pulse Rate, Glow Cycle

**Miss Streak**:
The number of consecutive Misses since the most recent Hit or Progress Reset.
_Avoid_: Miss Count, deaths

**Progress Reset**:
The state transition triggered by the third consecutive Miss that restores initial Progress and relocates the Target Zone.
_Avoid_: Game Over, Reset

**Miss Marker**:
The temporary three-red-pixel indicator centered on the Collision Span after a Miss.
_Avoid_: Red Explosion, Collision Marker

**Hit Animation**:
The whole-strip green visual shown after a Hit.
_Avoid_: Win Animation, Success Animation
