#ifndef HOOK_MANAGER_H
#define HOOK_MANAGER_H

class HookedAPI
{
public:
	HookedAPI();
	bool Hook(void *api, void *handler, void **result);
	void Unhook();
	void *old_handler() const { return old_handler_; }
private:
	static size_t page_size_;
	size_t size_;
	void *api_;
	void *old_handler_;
	uint32_t crc_;
};

class HookManager
{
public:
	HookManager();
	void Begin();
	void End();
	void *HookAPI(HMODULE dll, const char *api_name, void *handler, bool show_error = true, void **result = NULL);
	bool UnhookAPI(void * handler);
private:
	HookManager(const HookManager &src);
	HookManager & operator = (const HookManager &);// block the assignment operator
	void GetThreads();
	void SuspendThreads();
	void ResumeThreads();
	void FreeThreads();

	vector<HookedAPI *> api_list_;
	vector<HANDLE> thread_list_;
	size_t update_count_;
};

#endif