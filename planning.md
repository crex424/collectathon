# Development Planning / Implemenetation Documentation

Use this page to write down brainstorming features and planned features of the game. This page is also used to write down coding examples we learn and implemenet into the game as a reference.

## Understanding

This section is to be used as a guidebook for bugs and concepts we learn about during this game's development.

### General Terminology

* MIN_X is the border on the left of the x axis.
* MAX_X is the border on the right of the x axis.
* MIN_Y is the border of the top of the y axis.
* MAX_Y is the border of the bottom of the y axis.

### Butano Timers and Order of Calculation

When using `bn::timer` class and `bn::timers` class, always make sure to calculate elapsed time directly when using conditionals. This is because calulated values stored in a variable are essentially cached, and that value is always used until the variable re-calculates the value in the next game loop.
```c++
    int boosted_ticks_elapsed = timer_boost.elapsed_ticks();
    int boosted_seconds_passed = boosted_ticks_elapsed / bn::timers::ticks_per_second();
```
In this code, `int game_ticks_elapsed` stores the elapsed ticks of the timer at that very moment, and continues to use it until it's called again when the loop goes around. This can set a dangerous precedent when you need to precisely measure the exact elapsed time instead of a cached value, such as in this code example:
```c++
        // Boost Timer
        int boosted_ticks_elapsed = timer_boost.elapsed_ticks();
        int boosted_seconds_passed = boosted_ticks_elapsed / bn::timers::ticks_per_second();
        
        // Activate Boost
        if (bn::keypad::a_pressed() && boosters > 0 && !is_boosted)
        {
            boosters--;
            timer_boost.restart();
            is_boosted = true;
            player_speed = SPEED.integer() + 1;
        }

        // End Boost
        if (boosted_seconds_passed) >= 3 && is_boosted)
        {
            player_speed = SPEED.integer();
            is_boosted = false;
        }
```
When running this code, because we are calculating the value of `boosted_seconds_passed` first, that value gets cached and is then used in the second conditional statement `if (boosted_seconds_passed) >= 3 && is_boosted)`. If the value of `boosted_seconds_passed` was greater than three when it was calculated, the player's movement gets updated back to normal immediately, even though we called `timer_boost.restart()` in the first conditional. The solution is fairly simple: calculate the elapsed time directly into the conditional, like so:

```c++
        // End Boost
        if ((timer_boost.elapsed_ticks() / bn::timers::ticks_per_second()) >= 3 && is_boosted)
        {
            player_speed = SPEED.integer();
            is_boosted = false;
        }
```
Instead of using a cached value from `int boosted_seconds_passed`, we now directly calculate the elapsed time live, giving us an much more exact and precise value, preventing the boost mechanic from failing to update the player's movement as expected. This was quite the difficult bug to find, and the logic behind the bug is insidious, so caution is always key when dealing with game loops and cached values!

### Sprite Manipulation

## Planning required changes

Nothing at the moment!

## Brainstorming Game Feature Ideas

* Backdrop changes to a random color evert time the score is increased by 10.
* Add a way to recover boosts without resetting.
* Add a proper game timer and clean up the UI.
* If game timer is implemented, perhaps have it count down, allowing for the player to
  gain a high score?
* Add new treasure sprites that are worth more but only live for a very short time to
  incentivize using the boost even more.
* Maybe add moving treasure sprites? Or perhaps an enemy sprite that either
  defeats the player or steals treasure?
* Change Player and maybe treasure sprite to something more interesting.
* Change character sprite when score hits a certain threshhold.

## To Be Implemented
- [x] Add ability for the background color to change depending on score.
      This would display the change in level in a simple way.
- [x] Add a fade in and fade out to the level clear sprites.
- [x] Add simple animation to treasure sprite.
