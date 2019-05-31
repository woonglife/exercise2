#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/queue.h>

#define devide_sentence 100
#define num_thread 4
static LIST_HEAD(listhead, entry) head;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct listhead* headp = NULL;
int num_entries = 0;

struct entry {
	char name[BUFSIZ];
	int frequency;
	LIST_ENTRY(entry) entries;
};

char buf[num_thread][4096];

struct entry* np;;
struct entry* final_np = NULL;


void* function(void* arg) {
	int count = *(int*)arg;
	int line_length = strlen(buf[count]);
	int it;
	for (it = 0; it < line_length; ++it) {
		if (!isalnum(buf[count][it])) {
			buf[count][it] = ' ';
		}
		else {
			buf[count][it] = tolower(buf[count][it]);
		}
	}

	// Tokenization
	const char* WHITE_SPACE = " \t\n";
	char* tok = strtok(buf[count], WHITE_SPACE);

	if (tok == NULL || strcmp(tok, "") == 0) { // ��ū�Ѱ� ������ ���������� fgets�Ϸ�����.
		return NULL;
	}

	do {
		pthread_mutex_lock(&mutex);
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
				struct entry* e ;
				e= malloc(sizeof(struct entry));

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

		np = head.lh_first;
		int last_cmp;
		last_cmp = strcmp(tok, np->name);

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

		pthread_mutex_unlock(&mutex);
	} while (tok = strtok(NULL, WHITE_SPACE));//�� ���� ��ū�� Ȯ���Ѵ�.

}
int main(int argc, char** argv)
{
	if (argc != 2) {
		fprintf(stderr, "%s: not enough input\n", argv[0]);
		exit(1);
	}

	FILE* fp = fopen(argv[1], "r");

	LIST_INIT(&head);
	long long i = 0;


	int count = 0;

	while (fgets(buf[count], 4096, fp)) {

		pthread_t tid[num_thread];

		for (int l = 0; l < num_thread; l++) {
			pthread_create(&tid[l], NULL, function, &count++);
			i++;
		}
		for (int l = 0; l < num_thread; l++) {
			pthread_join(tid[l], NULL);
		}
		count = 0;
	}


	// Print the counting result very very slow way.
	int max_frequency = 0;
	struct entry* np;
	for (np = head.lh_first; np != NULL; np = np->entries.le_next) {
		if (max_frequency < np->frequency) {
			max_frequency = np->frequency;
		}
	}
	int it;
	for (it = max_frequency; it > 0; --it) {
		struct entry* np;
		for (np = head.lh_first; np != NULL; np = np->entries.le_next) {
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