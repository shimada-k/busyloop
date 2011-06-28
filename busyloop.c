#define _GNU_SOURCE

#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <sched.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

/* CPU番号とスレッドの数を指定してビジーループのスレッドを動かすプログラム */

#define u64 unsigned long long
#define U64_MAX	18446744073709551615ULL

u64 result;
char *progname;
int death_flag;

void usage(void)
{
	fprintf(stderr, "%s: [-c CPU#] [-N num_threads]\n",
		progname);
}

/* ビジーループスレッド */
void *thread_main(void *p)
{
	int pid;
	u64 cnt = 0ULL;

	pthread_t thread_id;
	thread_id = pthread_self();
	pid = getpid();

	printf("thread_id=%ld, pid=%d\n", thread_id, pid);

	for(;;){	/* カウンタをインクリメント */
		cnt++;
		
		if(result == U64_MAX || death_flag == 1){	/* U64_MAXに到達するか死亡フラグがたっていたら終了 */
			break;
		}

		result += cnt;
	}
}

int main(int argc, char *argv[])
{
	int thread_num = -1, target_cpu = -1, i;
	int nr_cpus = sysconf(_SC_NPROCESSORS_CONF);
	pthread_t *threads = NULL;
	int pid, signo;
	FILE *pid_f;
	sigset_t ss;
	cpu_set_t cset;

	if(!argc == 5){	/* cオプションとNオプションは絶対に必要 */
		usage();
		exit(EXIT_FAILURE);
	}
	else{
		for(i = 1; i < argc - 1; i++){	/* 引数でループ。argv[0]とarg[4]は飛ばす */
			if(strcmp(argv[i], "-N") == 0){
				thread_num = atoi(argv[i + 1]);
			}
			else if(strcmp(argv[i], "-c") == 0){
				target_cpu = atoi(argv[i + 1]);
			}
		}

		/* 引数の値のチェック */
		if(thread_num == -1 || !(-1 < target_cpu && target_cpu < nr_cpus)){
			usage();
			exit(EXIT_FAILURE);
		}

	}

	progname = argv[0];

	if(!(pid_f = fopen("busyloop.pid", "w"))){	/* .pidファイルを作成 */
		perror("fopen");
		exit(EXIT_FAILURE);
	}

	fprintf(pid_f, "%d", getpid());	/* pidを書き込む */
	fclose(pid_f);		/* *.pidファイルをクローズ */


	CPU_ZERO(&cset);
	CPU_SET(target_cpu, &cset);

	if((sched_setaffinity(0 , sizeof(cpu_set_t), &cset)) == -1){
		perror("sched_setaffinity");
		exit(EXIT_FAILURE);
	}

	printf("thread_num=%d, target_cpu=%d\n", thread_num, target_cpu);

	if((threads = calloc(thread_num, sizeof(pthread_t))) == NULL){
		puts("calloc(3) error");
		exit(EXIT_FAILURE);
	}

	for(i = 0; i < thread_num; i++){
		if((pthread_create(threads + i , NULL , thread_main , NULL)) == -1){
			puts("pthread_create(3) error");
			free(threads);
			exit(EXIT_FAILURE);
		}
	}

	/* シグナルハンドリングの準備 */
	sigemptyset(&ss);
	/* block SIGTERM */
	if(sigaddset(&ss, SIGTERM) == -1){
		puts("sigaddset(3) error");
		free(threads);
		exit(EXIT_FAILURE);
	}

	sigprocmask(SIG_BLOCK, &ss, NULL);

	for(;;){
		if(sigwait(&ss, &signo) == 0){	/* シグナルが受信できたら */
			if(signo == SIGTERM){
				death_flag= 1;
				puts("busyloop:sigterm recept");
				break;
			}
		}

		if(result == thread_num){	/* スレッドが終了したら */
			printf("%llu", result);
			break;
		}
	}

	for(i = 0; i < thread_num; i++){
		pthread_join(threads[i] , NULL);
	}

	free(threads);
	return 0;
}

