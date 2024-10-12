#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DICE 5
#define CATEGORIES 13

const char *categoryNames[CATEGORIES] = {
    "Ones", "Twos", "Threes", "Fours", "Fives", "Sixes",
    "Three of a Kind", "Four of a Kind", "Full House",
    "Small Straight", "Large Straight", "Yahtzee", "Chance"
};

/* Return -1 on EOF so redirected input can also end the game safely. */
int readNumber(int minimum, int maximum)
{
    char line[100];
    char *end;
    long number;
    int ch;

    while (1) {
        if (fgets(line, sizeof(line), stdin) == NULL) return -1;
        if (strchr(line, '\n') == NULL && !feof(stdin)) {
            while ((ch = getchar()) != '\n' && ch != EOF) {}
            printf("Input too long. Try again: ");
            continue;
        }
        number = strtol(line, &end, 10);
        if (end != line) {
            while (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n') end++;
            if (*end == '\0' && number >= minimum && number <= maximum)
                return (int)number;
        }
        printf("Enter a number from %d to %d: ", minimum, maximum);
    }
}

void rollDice(int dice[], int keep[])
{
    int i;
    for (i = 0; i < DICE; i++) {
        if (keep[i] == 0) dice[i] = rand() % 6 + 1;
    }
}

void displayDice(int dice[])
{
    int i;
    for (i = 0; i < DICE; i++) printf("%d:[%d]  ", i + 1, dice[i]);
    printf("\n");
}

int calculateScore(int dice[], int category)
{
    int counts[7] = {0};
    int sum = 0, largestGroup = 0;
    int pair = 0, triple = 0, run = 0, longestRun = 0;
    int i;

    for (i = 0; i < DICE; i++) {
        counts[dice[i]]++;
        sum += dice[i];
    }
    for (i = 1; i <= 6; i++) {
        if (counts[i] > largestGroup) largestGroup = counts[i];
        if (counts[i] == 2) pair = 1;
        if (counts[i] == 3) triple = 1;
        if (counts[i] > 0) run++;
        else run = 0;
        if (run > longestRun) longestRun = run;
    }

    if (category >= 0 && category <= 5) return counts[category + 1] * (category + 1);
    switch (category) {
        case 6: return largestGroup >= 3 ? sum : 0;
        case 7: return largestGroup >= 4 ? sum : 0;
        case 8: return pair && triple ? 25 : 0;
        case 9: return longestRun >= 4 ? 30 : 0;
        case 10: return longestRun >= 5 ? 40 : 0;
        case 11: return largestGroup == 5 ? 50 : 0;
        case 12: return sum;
        default: return 0;
    }
}

int totalScore(int scores[])
{
    int total = 0, i;
    for (i = 0; i < CATEGORIES; i++) {
        if (scores[i] >= 0) total += scores[i];
    }
    return total;
}

void displayScoreboard(int human[], int computer[])
{
    int i;
    printf("\n%-23s %8s %8s\n", "Category", "You", "Computer");
    for (i = 0; i < CATEGORIES; i++) {
        printf("%2d. %-19s", i + 1, categoryNames[i]);
        if (human[i] == -1) printf(" %8s", "-");
        else printf(" %8d", human[i]);
        if (computer[i] == -1) printf(" %8s\n", "-");
        else printf(" %8d\n", computer[i]);
    }
    printf("%-23s %8d %8d\n\n", "Total", totalScore(human), totalScore(computer));
}

int humanTurn(int dice[], int scores[])
{
    int keep[DICE] = {0};
    int roll, i, answer, category;

    printf("Your dice: ");
    displayDice(dice);
    for (roll = 2; roll <= 3; roll++) {
        printf("Roll again? (1 = yes, 0 = score now, 9 = quit): ");
        answer = readNumber(0, 9);
        while (answer > 1 && answer != 9) {
            printf("Choose 0, 1 or 9: ");
            answer = readNumber(0, 9);
        }
        if (answer == -1 || answer == 9) return 0;
        if (answer == 0) break;
        for (i = 0; i < DICE; i++) {
            printf("Keep die %d (value %d)? (1 = keep, 0 = reroll): ", i + 1, dice[i]);
            keep[i] = readNumber(0, 1);
            if (keep[i] == -1) return 0;
        }
        rollDice(dice, keep);
        printf("Your dice after roll %d: ", roll);
        displayDice(dice);
    }

    printf("\nAvailable categories:\n");
    for (i = 0; i < CATEGORIES; i++) {
        if (scores[i] == -1)
            printf("%2d. %-19s %d points\n", i + 1, categoryNames[i], calculateScore(dice, i));
    }
    while (1) {
        printf("Choose a category (1-13), or 0 to quit: ");
        answer = readNumber(0, 13);
        if (answer == -1 || answer == 0) return 0;
        category = answer - 1;
        if (scores[category] != -1) {
            printf("You already used that category.\n");
            continue;
        }
        scores[category] = calculateScore(dice, category);
        printf("You score %d in %s.\n", scores[category], categoryNames[category]);
        return 1;
    }
}

int bestCategory(int dice[], int scores[])
{
    int best = -1, bestScore = -1, score, i;
    for (i = 0; i < CATEGORIES; i++) {
        if (scores[i] == -1) {
            score = calculateScore(dice, i);
            if (score > bestScore) {
                bestScore = score;
                best = i;
            }
        }
    }
    return best;
}

/* Simple strategy: keep a completed combination, otherwise work towards
   an unused category. This is a heuristic, not a mathematically optimal AI. */
void computerKeep(int dice[], int scores[], int keep[])
{
    int counts[7] = {0};
    int target = 0, i, category, start, length, bestStart = 0, bestLength = 0;
    int kindAvailable = scores[6] == -1 || scores[7] == -1 || scores[8] == -1 || scores[11] == -1;

    for (i = 0; i < DICE; i++) {
        counts[dice[i]]++;
        keep[i] = 0;
    }
    category = bestCategory(dice, scores);
    if (category == -1) return;
    /* Preserve full houses, straights, Yahtzees, and strong kind scores. */
    if (calculateScore(dice, category) >= 25) {
        for (i = 0; i < DICE; i++) keep[i] = 1;
        return;
    }
    for (i = 1; i <= 6; i++) {
        if (kindAvailable || scores[i - 1] == -1) {
            if (target == 0 || counts[i] >= counts[target]) target = i;
        }
    }
    if (target != 0 && counts[target] >= 2) {
        for (i = 0; i < DICE; i++) keep[i] = dice[i] == target;
        return;
    }
    /* Keep one copy of each face in the most promising straight window. */
    if (scores[9] == -1 || scores[10] == -1) {
        int width = scores[10] == -1 ? 5 : 4;
        for (start = 1; start <= 7 - width; start++) {
            length = 0;
            for (i = start; i < start + width; i++) {
                if (counts[i] > 0) length++;
            }
            if (length > bestLength) {
                bestLength = length;
                bestStart = start;
            }
        }
        if (bestLength >= 3) {
            int used[7] = {0};
            for (i = 0; i < DICE; i++) {
                if (dice[i] >= bestStart && dice[i] < bestStart + width && !used[dice[i]]) {
                    keep[i] = 1;
                    used[dice[i]] = 1;
                }
            }
            return;
        }
    }
    for (i = 0; i < DICE; i++) {
        if (scores[12] == -1) keep[i] = dice[i] >= 4;
        else if (target != 0) keep[i] = dice[i] == target;
    }
}

void computerTurn(int dice[], int scores[])
{
    int keep[DICE], roll, i, kept, category;
    printf("\nComputer dice: ");
    displayDice(dice);
    for (roll = 2; roll <= 3; roll++) {
        computerKeep(dice, scores, keep);
        kept = 0;
        printf("Computer keeps positions:");
        for (i = 0; i < DICE; i++) {
            if (keep[i]) { printf(" %d", i + 1); kept++; }
        }
        if (kept == 0) printf(" none");
        printf("\n");
        if (kept == DICE) break;
        rollDice(dice, keep);
        printf("Computer dice after roll %d: ", roll);
        displayDice(dice);
    }
    category = bestCategory(dice, scores);
    if (category == -1) return;
    scores[category] = calculateScore(dice, category);
    printf("Computer scores %d in %s.\n", scores[category], categoryNames[category]);
}

int main(void)
{
    int human[CATEGORIES], computer[CATEGORIES];
    int humanDice[DICE], computerDice[DICE];
    int keep[DICE] = {0};
    int round, i, humanTotal, computerTotal;

    srand((unsigned int)time(NULL));
    for (i = 0; i < CATEGORIES; i++) {
        human[i] = -1;
        computer[i] = -1;
    }
    printf("YAHTZEE - You vs Computer\n");
    printf("13 rounds. Each category can be used once, even for zero points.\n");
    printf("No upper-section bonus or extra Yahtzee bonuses in this version.\n");

    for (round = 1; round <= CATEGORIES; round++) {
        printf("\n========== Round %d of 13 ==========\n", round);
        displayScoreboard(human, computer);
        rollDice(humanDice, keep);
        rollDice(computerDice, keep);
        if (!humanTurn(humanDice, human)) {
            printf("\nGame ended before completion.\n");
            return 0;
        }
        computerTurn(computerDice, computer);
    }
    printf("\n========== Final scores ==========\n");
    displayScoreboard(human, computer);
    humanTotal = totalScore(human);
    computerTotal = totalScore(computer);
    if (humanTotal > computerTotal) printf("You win!\n");
    else if (computerTotal > humanTotal) printf("Computer wins!\n");
    else printf("It is a tie!\n");
    return 0;
}
