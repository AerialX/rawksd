#include "riifs.h"

#include <pthread.h>
#include <dirent.h>

void NetworkInit()
{
}

OSLock CreateLock() {
	pthread_mutex_t *lock = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	if (lock)
		pthread_mutex_init(lock, NULL);
	return lock;
}

void GetLock(OSLock lock) {
	pthread_mutex_lock((pthread_mutex_t*)lock);
}

void ReleaseLock(OSLock lock) {
	pthread_mutex_unlock((pthread_mutex_t*)lock);
}

typedef struct {
	pthread_mutex_t thread_start;
	void (*thread_func)(void*);
	void *saved_arg;
} thread_arg;

void* thread_stub(void *_arg) {
	thread_arg *arg = (thread_arg*)_arg;
	void *t_arg = arg->saved_arg;
	void (*func)(void*) = arg->thread_func;
	pthread_mutex_lock(&arg->thread_start);
	pthread_mutex_unlock(&arg->thread_start);
	pthread_mutex_destroy(&arg->thread_start);
	free(arg);
	func(t_arg);
	return NULL;
}

void *Thread_Create(void *func, void *arg) {
	thread_arg *new_thread = (thread_arg*)malloc(sizeof(thread_arg));
	if (new_thread) {
		pthread_t tid;
		pthread_mutex_init(&new_thread->thread_start, NULL);
		pthread_mutex_lock(&new_thread->thread_start);
		new_thread->saved_arg = arg;
		new_thread->thread_func = (void(*)(void*))func;
		pthread_create(&tid, NULL, thread_stub, (void*)new_thread);
	}
	return (void*)new_thread;
}

void Thread_Start(void *_thread) {
	thread_arg *thread = (thread_arg*)_thread;
	pthread_mutex_unlock(&thread->thread_start);
}

class UnixDirectoryInfo : public DirectoryInfo
{
private:
	DIR *d;
public:
	UnixDirectoryInfo(string path) : d(NULL) {
		Exists = false;
		d = opendir(path.c_str());
		if (d) {
			Exists = true;
			Parent = path.substr(0, path.find_last_of("/"));
			Name = path.substr(path.find_last_of("/")+1);
		}
	}

	~UnixDirectoryInfo() {
		if (d)
			closedir(d);
	}

	Stat GetNext() {
		Stat st;
		struct stat sta;
		struct dirent *ent = readdir(d);

		if (ent==NULL)
			return st;

		string path = Parent + "/" + Name + "/" + ent->d_name;
		if (stat(path.c_str(), &sta)!=0 || (sta.st_mode & (S_IFDIR|S_IFREG))==0)
			return st;

		st.Mode = (sta.st_mode & S_IFMT)|S_IFREG;
		st.Name = ent->d_name;

		if (!(sta.st_mode & S_IFDIR)) {
			vector<string>::iterator iter = find(st.IDs.begin(), st.IDs.end(), path);
			st.Identifier = iter - st.IDs.begin();
			if (iter == st.IDs.end())
				st.IDs.push_back(path);
			st.Size = sta.st_size;
		}

		return st;
	}
};

DirectoryInfo* CreateDirectoryInfo(string path) {
	return new UnixDirectoryInfo(path);
}
