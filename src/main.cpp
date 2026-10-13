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
#include <bn_regular_bg_ptr.h>

#include "bn_regular_bg_items_lake_bg.h"
#include "bn_regular_bg_items_forest_bg.h"
#include "bn_sprite_items_treasure.h"
#include "bn_sprite_items_square.h"
#include "bn_sprite_items_boost.h"
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
static constexpr bn::size BOOST_SIZE = {8, 8};

// Full bounds of the screen
static constexpr int MIN_Y = -bn::display::height() / 2;
static constexpr int MAX_Y = bn::display::height() / 2;
static constexpr int MIN_X = -bn::display::width() / 2;
static constexpr int MAX_X = bn::display::width() / 2;
static constexpr int PLAYER_X = 25;
static constexpr int PLAYER_Y = -25;
static constexpr int TREASURE_X = 60;
static constexpr int TREASURE_Y = -45;
static constexpr int BOOST_X = 0;
static constexpr int BOOST_Y = 0;

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
static constexpr int LEVEL_Y = 0;

// Booster UI Element location
static constexpr int BOOSTER_X = -107;
static constexpr int BOOSTER_Y = -70;

// Booster Spawn Min / Max Wait Time
static constexpr int BOOST_WAIT_MIN = 10;
static constexpr int BOOST_WAIT_MAX = 15;

int main()
{
    bn::core::init();

    // Creates and starts various timers
    bn::timer timer_level_clear;

    // Pseudo-timer variables to bypass the 4 hardware timer limit on the GBA
    // Game Timer
    int timer_game = 0;
    int game_elapsed_seconds = 0;
    // Active Boost Timer
    int timer_boost_active = 0;
    int boost_active_elapsed_seconds = 0;
    // Boost Spawn Timer
    int timer_boost_spawn = 0;
    int boost_spawn_elapsed_seconds = 0;

    bn::random rng_treasure = bn::random();
    bn::random rng_boost = bn::random();

    // Will hold the sprites for the various UI elements
    bn::vector<bn::sprite_ptr, MAX_SCORE_CHARS> score_sprites = {};
    bn::vector<bn::sprite_ptr, MAX_BOOSTER_CHARS> booster_sprites = {};
    bn::vector<bn::sprite_ptr, MAX_LEVEL_CHARS> level_sprites = {};
    bn::vector<bn::sprite_ptr, MAX_TEST_CHARS> test_sprites = {};

    // Text Generator for UI Elements
    bn::sprite_text_generator text_generator(common::fixed_8x16_sprite_font);

    // Player and Treasure Sprites
    bn::sprite_ptr player = bn::sprite_items::square.create_sprite(PLAYER_X, PLAYER_Y);
    bn::sprite_ptr treasure = bn::sprite_items::treasure.create_sprite(TREASURE_X, TREASURE_Y);
    bn::sprite_ptr boost = bn::sprite_items::boost.create_sprite(BOOST_X, BOOST_Y);

    // Sprite attributes
    treasure.set_horizontal_scale(1);
    bn::fixed treasure_h_scale = treasure.horizontal_scale();
    boost.set_visible(false); // Hides boost sprite until needed

    // Dynamic Score Variables
    int score = 0;
    int previous_score = score;
    int max_score = 10;

    bn::string<MAX_SCORE_CHARS> score_string = bn::to_string<MAX_SCORE_CHARS>(score);
    text_generator.generate(SCORE_X, SCORE_Y, "SCORE " + score_string, score_sprites);

    // Dynamic Booster Variables
    int boosters = 3;
    int previous_boost_count = boosters;
    int player_speed = SPEED.integer();
    bool is_boosted = false;

    int boost_weight = 1; // Determines how strong the boost is
    int boost_spawn_wait_time = rng_boost.get_int(BOOST_WAIT_MIN, BOOST_WAIT_MAX);
    bool is_booster_spawned = false;

    bn::string<MAX_BOOSTER_CHARS> booster_string = bn::to_string<MAX_BOOSTER_CHARS>(boosters);
    text_generator.generate(BOOSTER_X, BOOSTER_Y, "BOOST " + booster_string, booster_sprites);

    // Stage Tracker
    Level level = Level::LAKE;
    Level previous_level = Level::LAKE;
    bool is_cleared = false; // Used for fade in and fade out
    bn::blending::set_transparency_alpha(0);
    bn::fixed previous_alpha = bn::blending::transparency_alpha();

    bn::string<MAX_LEVEL_CHARS> level_string = bn::to_string<MAX_LEVEL_CHARS>("Level 1 - Lake");

    text_generator.generate(LEVEL_X, LEVEL_Y, level_string, level_sprites);

    bool is_game_over = false;

    // Iterates through all vector sprite ptr elements to toggle blending on and set invisible
    for (int i = 0; i < level_sprites.size(); i++)
    {
        // Allows Sprite to be transparent or not
        level_sprites[i].set_blending_enabled(true);

        // Sets all vector sprite_ptr elements to be invisible
        level_sprites[i].set_visible(false);
    }

    // Treasure scale for animation
    bool is_treasure_growing = false;
    
    // Initializes backdrop to a white color as game loads
    bn::backdrop::set_color(bn::color(0, 0, 0));
    while (true)
    {
        // Treasure Scale
        treasure_h_scale = treasure.horizontal_scale();

        // Activate Boost
        if (bn::keypad::a_pressed() && boosters > 0 && !is_boosted)
        {
            boosters--;
            reset_timer(timer_boost_active, boost_active_elapsed_seconds);
            is_boosted = true;
            player_speed = SPEED.integer() + boost_weight;
        }

        // End Boost
        if (boost_active_elapsed_seconds >= 3 && is_boosted)
        {
            player_speed = SPEED.integer();
            is_boosted = false;
        }

        // Checks to see if boost should spawn or not
        if (boost_spawn_elapsed_seconds >= boost_spawn_wait_time && !is_boosted && !is_booster_spawned)
        {
            boost.set_visible(true);
            is_booster_spawned = true;
            reset_timer(timer_boost_spawn, boost_spawn_elapsed_seconds);
        }

        // Removes booster if player didn't collect it in time
        if (boost_spawn_elapsed_seconds >= 3 && is_booster_spawned)
        {
            boost.set_visible(false);
            is_booster_spawned = false;
            int new_x = rng_boost.get_int(MIN_X + 10, MAX_X - 10);
            int new_y = rng_boost.get_int(MIN_Y + 10, MAX_Y - 10);
            boost.set_position(new_x, new_y);
            reset_timer(timer_boost_spawn, boost_spawn_elapsed_seconds);
            rng_boost.update();
            boost_spawn_wait_time = rng_boost.get_int(BOOST_WAIT_MIN, BOOST_WAIT_MAX);
        }

        // LOGS FOR TESTING PURPOSES
        BN_LOG("Game Seconds Elapsed: ", game_elapsed_seconds);
        BN_LOG("Booster Wait Time in Seconds Elapsed: ", boost_spawn_elapsed_seconds);
        BN_LOG("Player Current Speed: ", player_speed);

        // Move player with d-pad
        if (bn::keypad::left_held() && !is_game_over)
        {
            if (player.x() < MIN_X)
            {
                player.set_x(MAX_X);
            }
            player.set_x(player.x() - player_speed);
        }
        if (bn::keypad::right_held() && !is_game_over)
        {
            if (player.x() > MAX_X)
            {
                player.set_x(MIN_X);
            }
            player.set_x(player.x() + player_speed);
        }
        if (bn::keypad::up_held() && !is_game_over)
        {
            if (player.y() < MIN_Y)
            {
                player.set_y(MAX_Y);
            }

            player.set_y(player.y() - player_speed);
        }
        if (bn::keypad::down_held() && !is_game_over)
        {
            if (player.y() > MAX_Y)
            {
                player.set_y(MIN_Y);
            }
            player.set_y(player.y() + player_speed);
        }

        // The bounding boxes of sprites, snapped to integer pixels
        bn::rect player_rect = bn::rect(player.x().round_integer(),
                                        player.y().round_integer(),
                                        PLAYER_SIZE.width(),
                                        PLAYER_SIZE.height());
        bn::rect treasure_rect = bn::rect(treasure.x().round_integer(),
                                          treasure.y().round_integer(),
                                          TREASURE_HITBOX_SIZE.width(),
                                          TREASURE_HITBOX_SIZE.height());
        bn::rect boost_rect = bn::rect(boost.x().round_integer(),
                                       boost.y().round_integer(),
                                       BOOST_SIZE.width(),
                                       BOOST_SIZE.height());

        // If the bounding boxes overlap, set the treasure or boost to a new location an increase score
        if (player_rect.intersects(treasure_rect))
        {
            // Jump to any random point in the screen
            int new_x = rng_treasure.get_int(MIN_X, MAX_X);
            int new_y = rng_treasure.get_int(MIN_Y, MAX_Y);
            treasure.set_position(new_x, new_y);
            score++;
        }
        if (player_rect.intersects(boost_rect) && is_booster_spawned)
        {
            boost.set_visible(false);
            is_booster_spawned = false;
            int new_x = rng_boost.get_int(MIN_X + 10, MAX_X - 10);
            int new_y = rng_boost.get_int(MIN_Y + 10, MAX_Y - 10);
            boost.set_position(new_x, new_y);
            reset_timer(timer_boost_spawn, boost_spawn_elapsed_seconds);
            rng_boost.update();
            boost_spawn_wait_time = rng_boost.get_int(BOOST_WAIT_MIN, BOOST_WAIT_MAX);
            boosters += 2;
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
        if (score >= max_score && !is_game_over)
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
            bn::regular_bg_ptr bg = bn::regular_bg_items::lake_bg.create_bg();
            level_string = bn::to_string<MAX_LEVEL_CHARS>("Level 1 - Lake");
            max_score = 10;
            boost_weight = 1;
            break;
        case Level::FOREST:
            bn::regular_bg_ptr bg = bn::regular_bg_items::forest_bg.create_bg();
            level_string = bn::to_string<MAX_LEVEL_CHARS>("Level 2 - Forest");
            max_score = 12;
            boost_weight = 2;
            break;
        case Level::PLATEAU:
            bn::backdrop::set_color(bn::color(163 >> 3, 67 >> 3, 26 >> 3));
            level_string = bn::to_string<MAX_LEVEL_CHARS>("Level 3 - Plateau");
            max_score = 14;
            boost_weight = 3;
            break;
        case Level::OCEAN:
            bn::backdrop::set_color(bn::color(24 >> 3, 54 >> 3, 201 >> 3));
            level_string = bn::to_string<MAX_LEVEL_CHARS>("Level 4 - Ocean");
            max_score = 16;
            boost_weight = 4;
            break;
        case Level::GAME_OVER:
            level_cleared_string = bn::to_string<MAX_LEVEL_CHARS>("GAME OVER!");
            level_sprites.clear();
            level_cleared_sprites.clear();
            text_generator.generate(LEVEL_CLEARED_X, LEVEL_CLEARED_Y, level_cleared_string, level_cleared_sprites);
            for (int i = 0; i < level_cleared_sprites.size(); i++)
                level_cleared_sprites[i].set_visible(true);
            is_game_over = true;
            break;
        }

        if (level != previous_level && !is_game_over)
        {
            level_sprites.clear();
            text_generator.generate(LEVEL_X, LEVEL_Y, level_string, level_sprites);

            for (int i = 0; i < level_sprites.size(); i++)
            {
                // Allows Sprite to be transparent or not
                level_sprites[i].set_blending_enabled(true);

                // Display sprites now that we need to
                level_sprites[i].set_visible(true);
            }
        }

        // When the level is cleared, fade in the level cleared sprites, and vice versa after three seconds have passed
        if (is_cleared && !is_game_over)
        {
            bn::blending::set_transparency_alpha(bn::min(bn::blending::transparency_alpha() + 0.0167, bn::fixed(1)));
        }
        else if (!is_game_over)
        {
            bn::blending::set_transparency_alpha(bn::max(bn::blending::transparency_alpha() - 0.0167, bn::fixed(0)));
        }
        if ((timer_level_clear.elapsed_ticks() / bn::timers::ticks_per_second()) >= 3)
        {
            is_cleared = false;
        }
        if (bn::blending::transparency_alpha() == 0 && bn::blending::transparency_alpha() != previous_alpha)
        {
            for (int i = 0; i < level_sprites.size(); i++)
            {
                // Display sprites now that we need to
                level_sprites[i].set_visible(false);
            }
        }

        // Reset Logic
        if (bn::keypad::start_pressed())
        {
            score = 0;
            boosters = 3;
            level = Level::LAKE;
            is_game_over = false;
            for (int i = 0; i < level_cleared_sprites.size(); i++)
                level_cleared_sprites[i].set_visible(false);

            // Instead of recreating player and treasure sprites every reset,
            // simply move them back to their initial positions
            player.set_position(PLAYER_X, PLAYER_Y);
            treasure.set_position(TREASURE_X, TREASURE_Y);
        }

        // Update Score Display
        score_string = bn::to_string<MAX_SCORE_CHARS>(score);
        if (score != previous_score && !is_game_over)
        {
            // Avoids wasting resources by only regenerating score text when score changes
            score_sprites.clear();
            text_generator.generate(SCORE_X, SCORE_Y,
                                    "SCORE " + score_string,
                                    score_sprites);
        }

        // Update booster resource display
        booster_string = bn::to_string<MAX_BOOSTER_CHARS>(boosters);
        if (boosters != previous_boost_count && !is_game_over)
        {
            booster_sprites.clear();
            text_generator.generate(BOOSTER_X, BOOSTER_Y,
                                    "BOOST " + booster_string,
                                    booster_sprites);
        }

        // Update RNG seed every frame so we don't get the same sequence of positions every time
        rng_treasure.update();
        rng_boost.update();

        // Updates previous couts of UI elements to current counts
        previous_score = score;
        previous_boost_count = boosters;
        previous_level = level;
        previous_alpha = bn::blending::transparency_alpha();

        treasure.set_horizontal_scale(treasure_h_scale);

        update_timer(timer_game, game_elapsed_seconds);
        update_timer(timer_boost_active, boost_active_elapsed_seconds);
        update_timer(timer_boost_spawn, boost_spawn_elapsed_seconds);

        bn::core::update();
    }
}