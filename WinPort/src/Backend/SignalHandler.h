class SignalHandler
{
	int _signo;
	struct _prev_sa;

protected:
	virtual void OnSignal(int, siginfo_t *) = 0;
	static void sOnSIgnal(int, siginfo_t *, void *);

public:
	SignalHandler(int signo);
	virtual ~SignalHandler();
};

struct sigaction {
               void     (*sa_handler)(int);
               void     (*sa_sigaction)(int, siginfo_t *, void *);
               sigset_t   sa_mask;
               int        sa_flags;
               void     (*sa_restorer)(void);
           };