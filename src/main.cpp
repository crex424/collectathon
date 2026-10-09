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
#include <bn_blending.h>
#include <bn_blending_actions.h>

#include "bn_sprite_items_dot.h"
#include "bn_sprite_items_square.h"
#include "common_fixed_8x16_font.h"
#include "bn_log.h"

#include "functions.hpp"
#include "Level.hpp"

// Pixels / Frame player moves at
static constexpr bn::fixed SPEED = 1;

// Width and height of the player and treasure bounding boxes
static constexpr bn::size PLAYER_SIZE = {8, 8};
static constexpr bn::size TREASURE_SIZE = {8, 8};
static constexpr bn::size TREASURE_HITBOX_SIZE = {8, 8};

// Full bounds of the screen
static constexpr int MIN_Y = -bn::display::height() / 2;
static constexpr int MAX_Y = bn::display::height() / 2;
static constexpr int MIN_X = -bn::display::width() / 2;
static constexpr int MAX_X = bn::display::width() / 2;
static constexpr int PLAYER_X = 25;
static constexpr int PLAYER_Y = -25;
static constexpr int TREASURE_X = 60;
static constexpr int TREASURE_Y = -45;

// Max character length for each UI element
static constexpr int MAX_SCORE_CHARS = 11;
static constexpr int MAX_BOOSTER_CHARS = 8;
static constexpr int MAX_LEVEL_CHARS = 23;
static constexpr int MAX_TEST_CHARS = 3;

// Score location
static constexpr int SCORE_X = 43;
static constexpr int SCORE_Y = -70;

// Level Location
static constexpr int LEVEL_X = -66;
static constexpr int LEVEL_Y = 70;

// Level Cleared Location
static constexpr int LEVEL_CLEARED_X = -64;
static constexpr int LEVEL_CLEARED_Y = 0;

// Booster resource location
static constexpr int BOOSTER_X = -107;
static constexpr int BOOSTER_Y = -70;

int main()
{
    bn::core::init();

    // Creates and starts various timers
    bn::timer timer_game;
    bn::timer timer_boost;
    bn::timer timer_level_clear;

    bn::random rng = bn::random();

    // Will hold the sprites for the various UI elements
    bn::vector<bn::sprite_ptr, MAX_SCORE_CHARS> score_sprites = {};
    bn::vector<bn::sprite_ptr, MAX_BOOSTER_CHARS> booster_sprites = {};
    bn::vector<bn::sprite_ptr, MAX_LEVEL_CHARS> level_sprites = {};
    bn::vector<bn::sprite_ptr, MAX_LEVEL_CHARS> level_cleared_sprites = {};
    bn::vector<bn::sprite_ptr, MAX_TEST_CHARS> test_sprites = {};

    // Text Generator for UI Elements
    bn::sprite_text_generator text_generator(common::fixed_8x16_sprite_font);

    // Player and Treasure Sprites
    bn::sprite_ptr player = bn::sprite_items::square.create_sprite(PLAYER_X, PLAYER_Y);
    bn::sprite_ptr treasure = bn::sprite_items::dot.create_sprite(TREASURE_X, TREASURE_Y);
    treasure.set_horizontal_scale(1);
    bn::fixed treasure_h_scale = treasure.horizontal_scale();

    // Dynamic Score Variables
    int score = 0;
    int previous_score = score;
    int max_score = 10;

    bn::string<MAX_SCORE_CHARS> score_string = bn::to_string<MAX_SCORE_CHARS>(score);
    text_generator.generate(SCORE_X, SCORE_Y, "SCORE " + score_string, score_sprites);

    // Non-static variables for the boost mechanic
    int boosters = 3;
    int previous_boost_count = boosters;
    int player_speed = SPEED.integer();
    bool is_boosted = false;

    bn::string<MAX_BOOSTER_CHARS> booster_string = bn::to_string<MAX_BOOSTER_CHARS>(boosters);
    text_generator.generate(BOOSTER_X, BOOSTER_Y, "BOOST " + booster_string, booster_sprites);

    // Stage Tracker
    Level level = Level::LAKE;
    Level previous_level = Level::LAKE;
    bool is_cleared = false; // Used for fade in and fade out
    bn::blending::set_transparency_alpha(0);
    bn::fixed previous_alpha = bn::blending::transparency_alpha();

    bn::string<MAX_LEVEL_CHARS> level_string = bn::to_string<MAX_LEVEL_CHARS>("Level 1 - Lake");
    bn::string<MAX_LEVEL_CHARS> level_cleared_string = bn::to_string<MAX_LEVEL_CHARS>("LEVEL 1 CLEARED");

    text_generator.generate(LEVEL_X, LEVEL_Y, level_string, level_sprites);
    text_generator.generate(LEVEL_CLEARED_X, LEVEL_CLEARED_Y, level_cleared_string, level_cleared_sprites);

    // Iterates through all vector sprite ptr elements to toggle blending on and set invisible
    for (int i = 0; i < level_cleared_sprites.size(); i++)
    {
        // Allows Sprite to be transparent or not
        level_cleared_sprites[i].set_blending_enabled(true);

        // Sets all vector sprite_ptr elements to be invisible
        level_cleared_sprites[i].set_visible(false);
    }

    // Treasure scale for animation
    bool is_treasure_growing = false;

    // Initializes backdrop to a white color as game loads
    bn::backdrop::set_color(bn::color(0, 0, 0));

    // Initializes level and level clear strings

    while (true)
    {
        // Game Timer
        int game_ticks_elapsed = timer_game.elapsed_ticks();
        int game_seconds_passed = game_ticks_elapsed / bn::timers::ticks_per_second();

        // Treasure Scale
        treasure_h_scale = treasure.horizontal_scale();

        // Activate Boost
        if (bn::keypad::a_pressed() && boosters > 0 && !is_boosted)
        {
            boosters--;
            timer_boost.restart();
            is_boosted = true;
            player_speed = SPEED.integer() + 1;
        }

        // End Boost
        if ((timer_boost.elapsed_ticks() / bn::timers::ticks_per_second()) >= 3 && is_boosted)
        {
            player_speed = SPEED.integer();
            is_boosted = false;
        }

        // LOGS FOR TESTING PURPOSES
        BN_LOG("Game Seconds Elapsed: ", game_seconds_passed);
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
                                          TREASURE_HITBOX_SIZE.width(),
                                          TREASURE_HITBOX_SIZE.height());

        // If the bounding boxes overlap, set the treasure to a new location an increase score
        if (player_rect.intersects(treasure_rect))
        {
            // Jump to any random point in the screen
            int new_x = rng.get_int(MIN_X, MAX_X);
            int new_y = rng.get_int(MIN_Y, MAX_Y);
            treasure.set_position(new_x, new_y);
            score++;
        }

        // Animates treasure sprite
        if (is_treasure_growing)
        {
            treasure_h_scale += 0.1;
            if (treasure_h_scale >= 1)
            {
                is_treasure_growing = false;
            }
        }
        if (!is_treasure_growing)
        {
            treasure_h_scale -= 0.1;
            if (treasure_h_scale <= 0.01)
            {
                is_treasure_growing = true;
            }
        }

        // Increases level counter when score reaches max
        if (score >= max_score)
        {
            score = 0;
            level_plus(level);
            is_cleared = true;
            timer_level_clear.restart();
        }

        // Simulates a change in Stages or Levels when score increases to max amount, up to level 4
        switch (level)
        {
        default:
        case Level::LAKE:
            bn::backdrop::set_color(bn::color(2, 19, 19));
            level_string = bn::to_string<MAX_LEVEL_CHARS>("Level 1 - Lake");
            max_score = 10;
            break;
        case Level::FOREST:
            bn::backdrop::set_color(bn::color(0, 19, 0));
            level_string = bn::to_string<MAX_LEVEL_CHARS>("Level 2 - Forest");
            level_cleared_string = bn::to_string<MAX_LEVEL_CHARS>("LEVEL 1 CLEARED!");
            max_score = 12;
            break;
        case Level::PLATEAU:
            bn::backdrop::set_color(bn::color(163 >> 3, 67 >> 3, 26 >> 3));
            level_string = bn::to_string<MAX_LEVEL_CHARS>("Level 3 - Plateau");
            level_cleared_string = bn::to_string<MAX_LEVEL_CHARS>("LEVEL 2 CLEARED!");
            max_score = 14;
            break;
        case Level::OCEAN:
            bn::backdrop::set_color(bn::color(24 >> 3, 54 >> 3, 201 >> 3));
            level_string = bn::to_string<MAX_LEVEL_CHARS>("Level 4 - Ocean");
            level_cleared_string = bn::to_string<MAX_LEVEL_CHARS>("LEVEL 3 CLEARED!");
            max_score = 9999;
            break;
        }

        if (level != previous_level)
        {
            level_sprites.clear();
            text_generator.generate(LEVEL_X, LEVEL_Y, level_string, level_sprites);

            level_cleared_sprites.clear();
            text_generator.generate(LEVEL_CLEARED_X, LEVEL_CLEARED_Y, level_cleared_string, level_cleared_sprites);

            for (int i = 0; i < level_cleared_sprites.size(); i++)
            {
                // Allows Sprite to be transparent or not
                level_cleared_sprites[i].set_blending_enabled(true);

                // Display sprites now that we need to
                level_cleared_sprites[i].set_visible(true);
            }
        }

        // When the level is cleared, fade in the level cleared sprites, and vice versa after three seconds have passed
        if (is_cleared)
        {
            bn::blending::set_transparency_alpha(bn::min(bn::blending::transparency_alpha() + 0.0167, bn::fixed(1)));
        }
        else
        {
            bn::blending::set_transparency_alpha(bn::max(bn::blending::transparency_alpha() - 0.0167, bn::fixed(0)));
        }
        if ((timer_level_clear.elapsed_ticks() / bn::timers::ticks_per_second()) >= 3)
        {
            is_cleared = false;
        }
        if (bn::blending::transparency_alpha() == 0 && bn::blending::transparency_alpha() != previous_alpha)
        {
            for (int i = 0; i < level_cleared_sprites.size(); i++)
            {
                // Display sprites now that we need to
                level_cleared_sprites[i].set_visible(false);
            }
        }

        // Reset Logic
        if (bn::keypad::start_pressed())
        {
            score = 0;
            boosters = 3;
            level = Level::LAKE;

            // Instead of recreating player and treasure sprites every reset,
            // simply move them back to their initial positions
            player.set_position(PLAYER_X, PLAYER_Y);
            treasure.set_position(TREASURE_X, TREASURE_Y);
        }

        // Update Score Display
        score_string = bn::to_string<MAX_SCORE_CHARS>(score);
        if (score != previous_score)
        {
            // Avoids wasting resources by only regenerating score text when score changes
            score_sprites.clear();
            text_generator.generate(SCORE_X, SCORE_Y,
                                    "SCORE " + score_string,
                                    score_sprites);
        }

        // Update booster resource display
        booster_string = bn::to_string<MAX_BOOSTER_CHARS>(boosters);
        if (boosters != previous_boost_count)
        {
            booster_sprites.clear();
            text_generator.generate(BOOSTER_X, BOOSTER_Y,
                                    "BOOST " + booster_string,
                                    booster_sprites);
        }

        // Update RNG seed every frame so we don't get the same sequence of positions every time
        rng.update();

        // Updates previous couts of UI elements to current counts
        previous_score = score;
        previous_boost_count = boosters;
        previous_level = level;
        previous_alpha = bn::blending::transparency_alpha();

        treasure.set_horizontal_scale(treasure_h_scale);

        bn::core::update();
    }
}