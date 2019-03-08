**tzschedule**

This is a ***crontab*** like task schedule system. Sadly it is not distributed, but it may suitable for our small-medium system.

I created this system mostly for the purpose of cooperating with [tzmonitor](https://github.com/taozhijiang/tzmonitor), as mentioned with its help we can collect large amount of realtime-ready information, but we also need to retrieve and check these data cyclically, and perhaps more frequently and timely more better, cause we can make behavior as soon as possible when something happens. If we achieve the goal by means of crontab, then the OS will create lots of process repeatly, and the frequency hard limited to one shot per minute; we can also hard code these in project, but the modifications may need the project rebuild and redeploy, can these code can not be re-used for other general purposes.   
In addition to these, crontab based task is hard to be maintained, because we usually write it and forget it,  and sometimes the task will not run due to lots of strange and hard-to-debug problem (like environment parameters problem, user password expires, and so on.

So I want to write a new task schedule system in C++. The function or features should be like:
1. crontab like time setting, flexible and powerful run time explaining.
2. task write in dynamic libraries, so can be dynamically load and unload, with out restart the whole system.
3. short task can run fast in thread pool, heavy task can defer run in additional thread separately.

