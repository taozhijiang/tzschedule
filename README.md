**tzschedule**

This is a ***crontab*** like task schedule system. Sadly it is not distributed, but it may suitable for our small-medium scaled system.

I created this system mostly for the purpose of cooperating with [tzmonitor](https://github.com/taozhijiang/tzmonitor), as mentioned with its help we can collect large amount of realtime-ready information, but we also need to retrieve and check these data periodically, and these behave more frequently and timely more better, cause we can make decisions as soon as possible when something happen. If we achieve the goal by means of crontab, then the OS will create lots of process repeatly, and the frequency limited to one shot per minute; then we can also hard code these in one project, but the modifications may need the project to be rebuilt and redeploied, and these code can not be re-used for other general purposes.
In addition to these, crontab-based task is hard to be maintained, because we usually write it and forget it (which is so common in script), and sometimes the task will not run due to lots of strange and hard-to-debug problem (like environment parameters problem, user password expires, and so on).

So I want to write a new task schedule system in C++, and the function or features should be like:
1. Crontab like time setting, flexible and powerful run time expression.
2. Task write in dynamic libraries, so can be dynamically load and unload, with out restart the whole system affecting other tasks.
3. Short task can run fast in thread pool, while heavy task can run in additional thread separately and asynchronously.

