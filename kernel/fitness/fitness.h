#ifndef FITNESS_H
#define FITNESS_H

typedef struct {
	const char* name;
	int duration;
	int rest;
} exercise_t;

void run_fitness_sequence(void);
void run_single_excercise(const exercise_t* ex);

#endif // !FITNESS_H
