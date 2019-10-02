class SignalHandler
{
	int _signo;
	struct _prev_sa;

protected:
	virtual void OnSignal(int, siginfo_t *) = 0;

public:

SignalHandler::SignalHandler(int signo)
	: _signo(signo)
{
}

SignalHandler::~SignalHandler()
{
}

void SignalHandler::sOnSIgnal(int, siginfo_t *, void *)
{
}

struct sigaction {
               void     (*sa_handler)(int);
               void     (*sa_sigaction)(int, siginfo_t *, void *);
               sigset_t   sa_mask;
               int        sa_flags;
               void     (*sa_restorer)(void);
           };