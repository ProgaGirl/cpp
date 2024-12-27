#include <iostream>
#include <unistd.h>
#include <cstdlib>

#include <string.h>
#include <vector>
#include <ctime>

#include <ncurses.h>

#include <thread>
#include <mutex>
#include <atomic>

using namespace std;

mutex mutex_screen;

//struct of file
//1. constant
//2. snake
//3. food
//4. bariers
//5. bonus
//6. gameplay
//7. main

//constant
enum { //for direction
        LEFT = 1,
        UP,
        RIGHT,
        DOWN
};

enum { //for snakes
        MAX_TAIL_SIZE = 1000,
        MIN_TAIL_SIZE = 3
};

enum { //for barier
        MAX_BARIER_SIZE = 20,
        BARIER_SECONDS = 50
};

enum { //for food
        MAX_FOOD_SIZE = 20,
        FOOD_SECONDS = 20
};

enum { //for bonus
        MAX_BONUS_SIZE = 20,
        BONUS_SECONDS = 20,
        TIME_TO_BONUS = 5
};

enum { //game element
        SPEED = 20000,
        SEED_NUMBER = 3,
        STOP_GAME = 'q'
};
//

//snake
struct Head {
        int x, y;
        int direction;
        char symbol = '@';
};

struct Tail {
        int x, y;
};

class Snake {
        public:
                Head head;
                size_t tail_size;
                char tail_symbol;
                vector<Tail> tail;

                bool bonus;
                time_t time_bonus;
                int speed;

                Snake(int valx, int valy, int vald, char symbol_t) {
                        head.x = valx; head.y = valy;
                        head.direction = vald;
                        tail_size = MIN_TAIL_SIZE + 1;
                        tail.resize(MAX_TAIL_SIZE, {0, 0});
                        tail_symbol = symbol_t;
                        bonus = false;
                        speed = 120;
                }

                void moveHead() {
                        int max_x = 0, max_y = 0;
                        getmaxyx(stdscr, max_y, max_x);
                        lock_guard<mutex> lock(mutex_screen);
                        mvprintw(head.y, head.x, " ");

                        switch (head.direction) {
                                case LEFT: head.x = (head.x <= 0) ? max_x - 1 : head.x - 1;
                                           break;
                                case RIGHT: head.x = (head.x >= max_x) ? 0 : head.x + 1;
                                            break;
                                case UP: head.y = (head.y <= 1) ? max_y - 1 : head.y - 1;
                                         break;
                                case DOWN: head.y = (head.y >= max_y) ? 0 : head.y + 1;
                                           break;
                        }
                        mvprintw(head.y, head.x, "%c", head.symbol);
                        refresh();
                }

                void moveTail() {
                        lock_guard<mutex> lock(mutex_screen);
                        mvprintw(tail[tail_size - 1].y, tail[tail_size - 1].x, " ");

                        for (size_t s = tail_size - 1; s > 0; s--) {
                                tail[s] = tail[s - 1];
                                if (tail[s].y || tail[s].x) {
                                    mvprintw(tail[s].y, tail[s].x, "%c", tail_symbol);
                                }
                        }

                        refresh();

                        tail[0].y = head.y;
                        tail[0].x = head.x;
                }

                void add_Tail() {
                        if (tail_size < MAX_TAIL_SIZE) {
                                tail_size++;
                        }
                }

                void sub_Tail() {
                        if (tail_size > MIN_TAIL_SIZE + 1) {
                                tail_size--;
                        }
                }

                bool isCrash() {
                        for (size_t s = 1; s < tail_size; s++) {
                                if (head.x == tail[s].x && head.y == tail[s].y) {
                                        return true;
                                }
                        }
                        return false;
                }

                bool isCrashHeadToTail(Snake& other_snake) {
                        for (size_t s = 1; s < other_snake.tail_size; s++) {
                                if (other_snake.head.x == tail[s].x && other_snake.head.y == tail[s].y) {
                                        return true;
                                }
                        }
                        return false;
                }

                bool isCrashTails(Snake& other_snake) {
                        for (size_t s = 1; s < tail_size; s++) {
                                if (other_snake.head.x == tail[s].x && other_snake.head.y == tail[s].y) {
                                        return true;
                                }
                        }
                        return false;
                }

                bool isCrashHeads(Snake& other_snake) {
                        if (head.x == other_snake.head.x && head.y == other_snake.head.y) {
                                return true;
                        }
                        return false;
                }

                void check_bonus(){
                        if (bonus && (time(nullptr) - time_bonus) > TIME_TO_BONUS){
                                mvprintw(10, 0, "^");
                                refresh();
                                bonus = false;
                                speed = 120;
                        }
                }

                int getLevel() const {
                        return (tail_size > MIN_TAIL_SIZE) ? (tail_size - MIN_TAIL_SIZE - 1) : 0;
                }
};
//

//food
struct Food {
        int x, y;
        time_t put_time;
        char symbol = 'F';
        bool enable;
};

class Foods {
        public:
                vector<Food> food;

                Foods() {
                        food.resize(MAX_FOOD_SIZE);
                        for (auto& f : food) {
                                f.x = 0; f.y = 0;
                                f.put_time = 0;
                                f.enable = false;
                        }
                }

                void putFoodSeed(Food& f) {
                        int max_x = 0, max_y = 0;
                        getmaxyx(stdscr, max_y, max_x);

                        lock_guard<mutex> lock(mutex_screen);

                        mvprintw(f.y, f.x, " ");

                        f.x = rand() % (max_x - 1);
                        f.y = rand() % (max_y - 3) + 2;
                        f.put_time = time(nullptr);
                        f.enable = true;
                        mvprintw(f.y, f.x, "%c", f.symbol);
                }

                void putFood() {
                        for (auto& f : food) {
                                putFoodSeed(f);
                        }
                }

                void refreshFood() {
                        for (auto& f : food) {
                                if (f.put_time && (!f.enable || (time(nullptr) - f.put_time) > FOOD_SECONDS)) {
                                        putFoodSeed(f);
                                }
                            }

                        lock_guard<mutex> lock(mutex_screen);
                        refresh();
                }

                void repairSeed(Snake& snake, Snake& other_snake) {
                        for (size_t s = 0; s < snake.tail_size; s++) {
                                for (auto& f : food) {
                                        if (f.x == snake.tail[s].x && f.y == snake.tail[s].y && f.enable) {
                                                putFoodSeed(f);
                                        }
                                }
                        }

                        for (size_t s = 0; s < other_snake.tail_size; s++) {
                                for (auto& f : food) {
                                        if (f.x == snake.tail[s].x && f.y == snake.tail[s].y && f.enable) {
                                                putFoodSeed(f);
                                        }
                                }
                        }

                        for (auto& f : food) {
                                if ((f.y == 0) ||
                                     ((f.y > 0 && f.y < 10) && (f.x > 0 && f.x < 20))) {
                                        putFoodSeed(f);
                                }
                        }
                }

                bool haveEat(Snake& snake) {
                        for (auto& f : food) {
                                if (f.enable && snake.head.x == f.x && snake.head.y == f.y) {
                                        lock_guard<mutex> lock(mutex_screen);
                                        f.enable = false;
                                        return true;
                                }
                        }
                        return false;
                }
};
//

//bariers
struct Barier {
        int x, y;
        time_t put_time;
        char symbol = 'X';
        bool enable = false;
};

class Bariers {
        public:
                std::vector<Barier> barier;

                Bariers() {
                        barier.resize(MAX_BARIER_SIZE);
                        for (auto& b : barier) {
                                b.x = 0; b.y = 0;
                                b.put_time = 0;
                        }
                }

                void putBarierSeed(Barier& b) {
                        int max_x = 0, max_y = 0;
                        getmaxyx(stdscr, max_y, max_x);

                        lock_guard<mutex> lock(mutex_screen);

                        mvprintw(b.y, b.x, " ");

                        b.x = rand() % (max_x - 7) + 4;
                        b.y = rand() % (max_y - 3) + 2;
                        b.put_time = time(nullptr);
                        b.enable = true;
                        mvprintw(b.y, b.x, "%c", b.symbol);
                }

                void putBarier() {
                        for (auto& b : barier) {
                                putBarierSeed(b);
                        }
                }

                void refreshBarier() {
                        for (auto& b : barier) {
                                if (b.put_time && (!b.enable || (time(nullptr) - b.put_time) > BARIER_SECONDS)) {
                                        putBarierSeed(b);
                                }
                        }
                        lock_guard<mutex> lock(mutex_screen);
                        refresh();
                }

                void repairBarier(Snake& snake, Snake& other_snake, Foods& fd) {
                        for (size_t s = 0; s < snake.tail_size; s++) {
                                for (auto& b : barier) {
                                        if (b.x == snake.tail[s].x && b.y == snake.tail[s].y && b.enable) {
                                                putBarierSeed(b);
                                        }
                                }
                        }

                        for (size_t s = 0; s < other_snake.tail_size; s++) {
                                for (auto& b : barier) {
                                        if (b.x == snake.tail[s].x && b.y == snake.tail[s].y && b.enable) {
                                                putBarierSeed(b);
                                        }
                                }
                        }

                        for (auto& b : barier) {
                                if ((b.y == 0) ||
                                     ((b.y > 0 && b.y < 10) && (b.x > 0 && b.x < 20))) {
                                                putBarierSeed(b);
                                }

                                for (auto& f : fd.food) {
                                    if (b.x == f.x && b.y == f.y && b.enable && f.enable) {
                                                putBarierSeed(b);
                                    }
                                }
                        }
                }

                bool iscrashBarier(Snake& snake) {
                        for (auto& b : barier) {
                                if (b.enable && snake.head.x == b.x && snake.head.y == b.y) {
                                        return true;
                                }
                        }
                        return false;
                }
};
//

//bonuses
struct Bonus {
        int x, y;
        time_t put_time;
        char symbol = 'B';
        bool enable = false;
};

class Bonuses {
        public:
                Bonus bonus;

                Bonuses () {
                    bonus.x = 0; bonus.y = 0;
                    bonus.put_time = 0;
                }

                void putBonus() {
                        int max_x = 0, max_y = 0;
                        getmaxyx(stdscr, max_y, max_x);

                        lock_guard<mutex> lock(mutex_screen);

                        mvprintw(bonus.y, bonus.x, " ");

                        bonus.x = rand() % (max_x - 5) + 3;
                        bonus.y = rand() % (max_y - 8) + 1;
                        bonus.put_time = time(nullptr);
                        bonus.enable = true;
                        mvprintw(bonus.y, bonus.x, "%c", bonus.symbol);
                }

                void refreshBonus() {
                        if (bonus.put_time && (!bonus.enable || (time(nullptr) - bonus.put_time) > BONUS_SECONDS)) {
                                putBonus();
                        }
                        lock_guard<mutex> lock(mutex_screen);
                        refresh();
                }

                void repairBonus(Snake& snake, Snake& other_snake, Foods& fd, Bariers& br) {
                        for (size_t s = 0; s < snake.tail_size; s++) {
                                if (bonus.x == snake.tail[s].x && bonus.y == snake.tail[s].y && bonus.enable) {
                                        putBonus();
                                }
                        }

                        for (size_t s = 0; s < other_snake.tail_size; s++) {
                                if (bonus.x == snake.tail[s].x && bonus.y == snake.tail[s].y && bonus.enable) {
                                        putBonus();
                                }
                        }

                        if ((bonus.y == 0) || ((bonus.y > 0 && bonus.y < 10) && (bonus.x > 0 && bonus.x < 20))) {
                                putBonus();
                        }

                        for (auto& f : fd.food) {
                                if (bonus.x == f.x && bonus.y == f.y && bonus.enable && f.enable) {
                                        putBonus();
                                }
                        }

                        for (auto& ba : br.barier) {
                                if (bonus.x == ba.x && bonus.y == ba.y && bonus.enable && ba.enable) {
                                        putBonus();
                                }
                        }
                }

                void haveBonus(Snake& snake) {
                        if (bonus.enable && snake.head.x == bonus.x && snake.head.y == bonus.y) {
                                lock_guard<mutex> lock(mutex_screen);
                                bonus.enable = false;
                                snake.bonus = true;
                                snake.time_bonus = time(nullptr);
                                snake.speed = 40;
                                mvprintw(10, 0, "&");
                                refresh();
                        }
                }
};
//

//gameplay
class Gameplay {
        public:
                Snake snake1, snake2;
                Foods all_food;
                Bariers all_barier;
                Bonuses all_bonus;

                atomic<bool> isRun {true};
                int key_press = 0;
                bool flag_crash = false;

                Gameplay() : snake1(0, 4, RIGHT, '*'), snake2(0, 6, RIGHT, '+') {
                        srand(time(nullptr));

                        initscr();
                        keypad(stdscr, TRUE);
                        raw();
                        noecho();
                        curs_set(FALSE);

                        timeout(10);

                        all_food.putFood();
                        all_barier.putBarier();
                        all_bonus.putBonus();
                }

                void changeDirection_snake(int key) {
                        lock_guard<mutex> lock(mutex_screen);

                        switch (key) {
                                //snake1
                                case KEY_DOWN: if (snake1.head.direction != UP) {snake1.head.direction = DOWN;}
                                               break;
                                case KEY_UP: if (snake1.head.direction != DOWN) {snake1.head.direction = UP;}
                                             break;
                                case KEY_LEFT: if (snake1.head.direction != RIGHT) {snake1.head.direction = LEFT;}
                                               break;
                                case KEY_RIGHT: if (snake1.head.direction != LEFT) {snake1.head.direction = RIGHT;}
                                                break;
                                //snake2
                                case 's': if (snake2.head.direction != UP) {snake2.head.direction = DOWN;}
                                          break;
                                case 'w': if (snake2.head.direction != DOWN) {snake2.head.direction = UP;}
                                          break;
                                case 'a': if (snake2.head.direction != RIGHT) {snake2.head.direction = LEFT;}
                                          break;
                                case 'd': if (snake2.head.direction != LEFT) {snake2.head.direction = RIGHT;}
                                          break;
                        }
                }

                //thread for snake
                void thread_snake(Snake& snake, Snake& other_snake, Foods& f, Bariers& ba, Bonuses& bo) {
                        while (isRun) {
                                snake.check_bonus();

                                if (snake.isCrashHeadToTail(other_snake)) {
                                        flag_crash = true;
                                        other_snake.sub_Tail();
                                }

                                if ((snake.isCrash() || snake.isCrashTails(other_snake) ||
                                     snake.isCrashHeads(other_snake) || ba.iscrashBarier(snake)) &&
                                    !flag_crash) {
                                        lock_guard<mutex> lock(mutex_screen);
                                        isRun = false;
                                        break;
                                } else {
                                        flag_crash = false;
                                }

                                snake.moveHead();
                                snake.moveTail();

                                if (f.haveEat(snake)) {
                                        snake.add_Tail();
                                }


                                bo.haveBonus(snake);
                                this_thread::sleep_for(chrono::milliseconds(snake.speed));
                        }
                }

                //thread for food
                void thread_food(Foods& f, Snake& snake, Snake& other_snake) {
                        while (isRun) {
                                f.repairSeed(snake, other_snake);
                                f.refreshFood();

                                this_thread::sleep_for(chrono::milliseconds(100));
                        }
                }

                //thread for barier
                void thread_barier(Bariers& b, Snake& snake, Snake& other_snake, Foods& f) {
                        while (isRun) {
                                b.repairBarier(snake, other_snake, f);
                                b.refreshBarier();

                                this_thread::sleep_for(chrono::milliseconds(100));
                        }
                }

                //thread for bonus
                void thread_bonus(Bonuses& b, Snake& snake, Snake& other_snake, Foods& f, Bariers& br) {
                        while (isRun) {
                                b.repairBonus(snake, other_snake, f, br);
                                b.refreshBonus();

                                this_thread::sleep_for(chrono::milliseconds(100));
                        }
                }

                void displayLevels() {
                        lock_guard<mutex> lock(mutex_screen);
                        mvprintw(0, 0, " Press 'q' for EXIT");
                        mvprintw(3, 0, " Snake1 Level: %d", snake1.getLevel());
                        mvprintw(5, 0, " Snake2 Level: %d", snake2.getLevel());
                        refresh();
                }

                void exitLevel() {
                        int level1 = snake1.getLevel();
                        int level2 = snake2.getLevel();
                        clear();
                        refresh();

                        displayLevels();
                        mvprintw(10, 0, "Game Over!");
                        if (level1 > level2) {
                                mvprintw(11, 0, "Winner: Snake1 with Level %d", level1);
                        } else if (level2 > level1) {
                                mvprintw(11, 0, "Winner: Snake2 with Level %d", level2);
                        } else {
                                mvprintw(11, 0, "Both at Level %d", level1);
                        }
                        refresh();
                }

                void run() {
                        thread thread_snake1(&Gameplay::thread_snake, this, ref(snake1), ref(snake2), ref(all_food), ref(all_barier), ref(all_bonus));
                        thread thread_snake2(&Gameplay::thread_snake, this, ref(snake2), ref(snake1), ref(all_food), ref(all_barier), ref(all_bonus));
                        thread thread_f(&Gameplay::thread_food, this, ref(all_food), ref(snake1), ref(snake2));
                        thread thread_br(&Gameplay::thread_barier, this, ref(all_barier), ref(snake1), ref(snake2), ref(all_food));
                        thread thread_bo(&Gameplay::thread_bonus, this, ref(all_bonus), ref(snake1), ref(snake2), ref(all_food), ref(all_barier));

                        while (key_press != STOP_GAME && isRun) {
                                key_press = getch();
                                changeDirection_snake(key_press);
                                displayLevels();
                        }

                        isRun = false;

                        thread_snake1.join();
                        thread_snake2.join();
                        thread_f.join();
                        thread_br.join();
                        thread_bo.join();

                        exitLevel();

                        key_press = 0;
                        while (key_press != STOP_GAME) {
                                key_press = getch();
                                this_thread::sleep_for(chrono::milliseconds(1));
                        }
                        endwin();
                }
};

int main() {
    Gameplay gameplay;
    gameplay.run();

    return 0;
}
