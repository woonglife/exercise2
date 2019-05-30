#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <sys/queue.h>

#define devide_sentence 100

static LIST_HEAD(listhead, entry) head;

struct listhead* headp = NULL;
int num_entries = 0;

struct entry {
	char name[BUFSIZ];
	int frequency;
	LIST_ENTRY(entry) entries;
};

char buf[][4096] = {};

struct entry* np = head.lh_first;
struct entry* final_np = NULL;

pthread_mutex_t mutex = PTRHEAD_MUTEX_INITIALIZATION;


void* fucntion(void* arg) {
	long long i = *(long long*)arg;
	i *= devide_sentence;
	int count = 0;
	while (count++ < devide_sentence) {

		int line_length = strlen(buf[i + count]);

		for (int it = 0; it < line_length; ++it) {
			if (!isalnum(buf[i + count][it])) {
				buf[i + count][it] = ' ';
			}
			else {
				buf[i + count][it] = tolower(buf[i + count][it]);
			}
		}

		// Tokenization
		const char* WHITE_SPACE = " \t\n";
		char* tok = strtok(buf[i + count], WHITE_SPACE);

		if (tok == NULL || strcmp(tok, "") == 0) { // 토큰한게 끝나면 다음문장을 fgets하러간다.
			continue;
		}

		do {
			pthread_mutex_lock(&mutex)
			if (num_entries == 0) {
				struct entry* e = malloc(sizeof(struct entry));

				strncpy(e->name, tok, strlen(tok));
				e->frequency = 1;

				LIST_INSERT_HEAD(&head, e, entries);
				num_entries++;
				continue;
			}
			else if (num_entries == 1) {
				int cmp = strcmp(tok, head.lh_first->name);

				if (cmp == 0) {
					head.lh_first->frequency++;
				}
				else if (cmp > 0) {
					struct entry* e = malloc(sizeof(struct entry));

					strncpy(e->name, tok, strlen(tok));
					e->frequency = 1;


					LIST_INSERT_AFTER(head.lh_first, e, entries);
					num_entries++;
				}
				else if (cmp < 0) {
					struct entry* e = malloc(sizeof(struct entry));

					strncpy(e->name, tok, strlen(tok));
					e->frequency = 1;

					LIST_INSERT_HEAD(&head, e, entries);
					num_entries++;
				}

				continue;
			}

			// Reduce: actual word-counting


			int last_cmp = strcmp(tok, np->name);

			if (last_cmp < 0) {
				struct entry* e = malloc(sizeof(struct entry));

				strncpy(e->name, tok, strlen(tok));
				e->frequency = 1;

				LIST_INSERT_HEAD(&head, e, entries);
				num_entries++;

				continue;

			}
			else if (last_cmp == 0) {
				np->frequency++;

				continue;
			}
			for (np = np->entries.le_next; np != NULL; np = np->entries.le_next) {
				int cmp = strcmp(tok, np->name);

				if (cmp == 0) {
					np->frequency++;
					break;
				}
				else if (last_cmp * cmp < 0) { // sign-crossing occurred
					struct entry* e = malloc(sizeof(struct entry));

					strncpy(e->name, tok, strlen(tok));
					e->frequency = 1;

					LIST_INSERT_BEFORE(np, e, entries);
					num_entries++;

					break;
				}

				if (np->entries.le_next == NULL) {
					final_np = np;
				}
				else {
					last_cmp = cmp;
				}
			}
			if (!np && final_np) {
				struct entry* e = malloc(sizeof(struct entry));

				strncpy(e->name, tok, strlen(tok));
				e->frequency = 1;

				LIST_INSERT_AFTER(final_np, e, entries);
				num_entries++;
			}
			
			pthread_mutex_unlock(&mutex)
		} while (tok = strtok(NULL, WHITE_SPACE));//그 다음 토큰을 확인한다.
	}
	pthread_exit(NULL);
}
int main(int argc, char** argv)
{
	if (argc != 2) {
		fprintf(stderr, "%s: not enough input\n", argv[0]);
		exit(1);
	}

	FILE* fp = fopen(argv[1], "r");
	
	pthread_t tid[];
	LIST_INIT(&head);
	long long i = 0;
	long long num_thread = 0;;
	while (fgets(buf[i], 4096, fp)) {
		if (i % devide_sentence == 0) {
			pthread_create(&tid[i], NULL, function, &num_thread);
			num_thread++;
		}
		i++;
	}

	for (int j = 0; j < i; j++) {
		pthread_join(tid[j], NULL);
	}


	// Print the counting result very very slow way.
	int max_frequency = 0;

	for (struct entry* np = head.lh_first; np != NULL; np = np->entries.le_next) {
		if (max_frequency < np->frequency) {
			max_frequency = np->frequency;
		}
	}

	for (int it = max_frequency; it > 0; --it) {
		for (struct entry* np = head.lh_first; np != NULL; np = np->entries.le_next) {
			if (np->frequency == it) {
				printf("%s %d\n", np->name, np->frequency);
			}
		}
	}

	// Release
	while (head.lh_first != NULL) {
		LIST_REMOVE(head.lh_first, entries);
	}

	fclose(fp);

	return 0;
}