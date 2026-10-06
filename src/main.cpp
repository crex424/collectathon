#include <bn_core.h>
#include <bn_display.h>
#include <bn_log.h>
#include <bn_keypad.h>
#include <bn_random.h>
#include <bn_rect.h>
#include <bn_sprite_ptr.h>
#include <bn_sprite_text_generator.h>
#include <bn_size.h>
#include <bn_string.h>
#include <bn_backdrop.h>
#include <bn_timers.h>
#include <bn_timer.h>

#include "bn_sprite_items_dot.h"
#include "bn_sprite_items_square.h"
#include "common_fixed_8x16_font.h"
#include "bn_log.h"

// Pixels / Frame player moves at
static constexpr bn::fixed SPEED = 1;

// Width and height of the the player and treasure bounding boxes
static constexpr bn::size PLAYER_SIZE = {8, 8};
static constexpr bn::size TREASURE_SIZE = {8, 8};

// Full bounds of the screen
static constexpr int MIN_Y = -bn::display::height() / 2;
static constexpr int MAX_Y = bn::display::height() / 2;
static constexpr int MIN_X = -bn::display::width() / 2;
static constexpr int MAX_X = bn::display::width() / 2;
static constexpr int PLAYER_X = 25;
static constexpr int PLAYER_Y = -25;
static constexpr int TREASURE_X = 60;
static constexpr int TREASURE_Y = -45;

// Number of characters required to show the longest numer possible in an int (-2147483647)
static constexpr int MAX_SCORE_CHARS = 11;
static constexpr int MAX_BOOSTER_CHARS = 1;
static constexpr int MAX_TEST_CHARS = 3;

// Score location
static constexpr int SCORE_X = 70;
static constexpr int SCORE_Y = -70;

// Booster resource location
static constexpr int BOOSTER_X = -80;
static constexpr int BOOSTER_Y = -70;

int main()
{
    bn::core::init();

    // Creates and starts a timer for boost mechanic
    bn::timer timer_game;
    bn::timer timer_boost;

    bn::random rng = bn::random();

    // Will hold the sprites for the score
    bn::vector<bn::sprite_ptr, MAX_SCORE_CHARS> score_sprites = {};
    bn::sprite_text_generator text_generator(common::fixed_8x16_sprite_font);

    bn::vector<bn::sprite_ptr, MAX_BOOSTER_CHARS> booster_sprites = {};
    bn::vector<bn::sprite_ptr, MAX_TEST_CHARS> test_sprites = {};

    int score = 0;

    // Everything related to boosters, including amout left and modifiers
    int boosters = 3;
    int player_speed = SPEED.integer();
    bool is_boosted = false;

    bn::sprite_ptr player = bn::sprite_items::square.create_sprite(PLAYER_X, PLAYER_Y);
    bn::sprite_ptr treasure = bn::sprite_items::dot.create_sprite(TREASURE_X, TREASURE_Y);
    bn::backdrop::set_color(bn::color(0, 31, 31));

    while (true)
    {
        // Game Timer
        int game_ticks_elapsed = timer_game.elapsed_ticks();
        int game_seconds_passed = game_ticks_elapsed / bn::timers::ticks_per_second();

        // Timer used specifically for the boost mechanic
        int boosted_ticks_elapsed = timer_boost.elapsed_ticks();
        int boosted_seconds_passed = boosted_ticks_elapsed / bn::timers::ticks_per_second();

                if (bn::keypad::a_pressed() && boosters > 0 && !is_boosted)
        {
            boosters--;
            timer_boost.restart();
            is_boosted = true;
            player_speed = SPEED.integer() + 5;
        }
        if (boosted_seconds_passed >= 3 && is_boosted)
        {
            player_speed = SPEED.integer();
            is_boosted = false;
        }

        // LOGS FOR TESTING PURPOSES
        BN_LOG("Game Ticks Elapsed: ", game_seconds_passed);
        BN_LOG("Booster Ticks Elapsed: ", boosted_seconds_passed);
        BN_LOG("Player Current Speed: ", player_speed);

        // Move player with d-pad
        if (bn::keypad::left_held())
        {
            if (player.x() < MIN_X)
            {
                player.set_x(MAX_X);
            }
            player.set_x(player.x() - player_speed);
        }
        if (bn::keypad::right_held())
        {
            if (player.x() > MAX_X)
            {
                player.set_x(MIN_X);
            }
            player.set_x(player.x() + player_speed);
        }
        if (bn::keypad::up_held())
        {
            if (player.y() < MIN_Y)
            {
                player.set_y(MAX_Y);
            }

            player.set_y(player.y() - player_speed);
        }
        if (bn::keypad::down_held())
        {
            if (player.y() > MAX_Y)
            {
                player.set_y(MIN_Y);
            }
            player.set_y(player.y() + player_speed);
        }

        // The bounding boxes of the player and treasure, snapped to integer pixels
        bn::rect player_rect = bn::rect(player.x().round_integer(),
                                        player.y().round_integer(),
                                        PLAYER_SIZE.width(),
                                        PLAYER_SIZE.height());
        bn::rect treasure_rect = bn::rect(treasure.x().round_integer(),
                                          treasure.y().round_integer(),
                                          TREASURE_SIZE.width(),
                                          TREASURE_SIZE.height());

        // If the bounding boxes overlap, set the treasure to a new location an increase score
        if (player_rect.intersects(treasure_rect))
        {
            // Jump to any random point in the screen
            int new_x = rng.get_int(MIN_X, MAX_X);
            int new_y = rng.get_int(MIN_Y, MAX_Y);
            treasure.set_position(new_x, new_y);

            score++;
        }

        // Update score display
        bn::string<MAX_SCORE_CHARS> score_string = bn::to_string<MAX_SCORE_CHARS>(score);
        if (bn::keypad::start_pressed())
        {
            score = 0;
            boosters = 3;
            player = bn::sprite_items::square.create_sprite(PLAYER_X, PLAYER_Y);
            treasure = bn::sprite_items::dot.create_sprite(TREASURE_X, TREASURE_Y);
        }
        score_sprites.clear();
        text_generator.generate(SCORE_X, SCORE_Y,
                                score_string,
                                score_sprites);

        bn::string<MAX_BOOSTER_CHARS> booster_string = bn::to_string<MAX_BOOSTER_CHARS>(boosters);
        booster_sprites.clear();
        text_generator.generate(BOOSTER_X, BOOSTER_Y,
                                booster_string,
                                booster_sprites);

        // FOR TESTING PURPOSES ONLY; May be used as a game timer in the future!
        bn::string<MAX_TEST_CHARS> test_string = bn::to_string<MAX_TEST_CHARS>(boosted_seconds_passed);
        test_sprites.clear();
        text_generator.generate(0, -70,
                                test_string,
                                test_sprites);

        // Update RNG seed every frame so we don't get the same sequence of positions every time
        rng.update();

        bn::core::update();
    }
}