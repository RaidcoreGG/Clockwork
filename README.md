# Clockwork++

A lightweight asynchronous task scheduling library designed for high-performance applications and game engines.

---
## The Pitch

Clockwork++ was primarily designed for game engines and applications that require fine-grained control over asynchronous work. Unlike traditional thread pool implementations, Clockwork++ supports multiple independent thread pools, allowing workloads to be separated by responsibility.

For example, dedicated pools can be created for:

* Networking / Socket operations  
* File I/O  
* Asset streaming  
* Physics simulation  
* General compute workloads  
* Background maintenance tasks  

Tasks can be dispatched to a specific pool and assigned a priority, ensuring critical work is processed ahead of lower-priority jobs.

---
## Features

* Multiple independent thread pools
* Task prioritization
* Awaitable task handles
* Simple async execution API
* Reusable dispatchers
* Recurring scheduled tasks
* Minimal dependencies
* Designed for engine and real-time application workloads

---

## Getting Started

### Creating the Context

Before dispatching any work, a Clockwork context must be created.

```cpp
using namespace Raidcore::Clockwork;

// Create N thread pools with automatic thread counts.
Context::Create(4);

// Or specify both pool count and threads per pool.
Context::Create(4, 2);

When shutting down:

```cpp
Context::Destroy();
```

---

## Simple Asynchronous Operations

Use `Run()` to execute work asynchronously and receive an awaitable task handle.

```cpp
auto task = Run<int>(
	ETaskPriority::Normal,
	[]()
	{
		return 42;
	}
);

int result = task->Await();
```

You can also dispatch directly to a specific pool:

```cpp
auto task = Run<std::string>(
	1,
	ETaskPriority::High,
	[]()
	{
		return "Hello Clockwork";
	}
);

std::string value = task->Await();
```

---

## Task Priorities

Clockwork++ supports prioritized execution:

| Priority  | Purpose                      |
| --------- | ---------------------------- |
| Immediate | Time-sensitive engine work   |
| High      | Networking, gameplay systems |
| Normal    | General application tasks    |
| Low       | Background processing        |

### Example

```cpp
Run<void>(
	ETaskPriority::Immediate,
	[]()
	{
		ProcessNetworkMessages();
	}
);

Run<void>(
	ETaskPriority::Low,
	[]()
	{
		RebuildCache();
	}
);
```

---

## Dispatchers

A `Dispatcher<T>` stores a reusable asynchronous operation together with its target thread pool and priority.

### Creating a Dispatcher

```cpp
Dispatcher<int> dispatcher(
	[]()
	{
		return PerformExpensiveCalculation();
	},
	2, // Pool ID
	ETaskPriority::High
);
```

### Dispatching Work

```cpp
auto task = dispatcher.Dispatch();
int result = task->Await();
```

Or call it directly:

```cpp
auto task = dispatcher();
```

### Reconfiguring a Dispatcher

```cpp
dispatcher.SetPool(1);
dispatcher.SetPriority(ETaskPriority::Critical);

dispatcher.SetAction([]()
{
	return GenerateData();
});
```

---

## Scheduled Tasks

Scheduled tasks allow you to run recurring work at fixed intervals using the Clockwork context.

They are ideal for:

* Engine tick systems
* Network polling
* Heartbeats
* Cleanup jobs
* Periodic background maintenance

---

### Scheduling API

```cpp
static std::shared_ptr<ScheduledTask> Schedule(
	uint32_t aPool,
	std::chrono::milliseconds aInterval,
	Action<void> aAction,
	std::chrono::steady_clock::time_point aFirstExecution =
		std::chrono::steady_clock::now()
);
```

```cpp
static std::shared_ptr<ScheduledTask> Schedule(
	std::chrono::milliseconds aInterval,
	Action<void> aAction,
	std::chrono::steady_clock::time_point aFirstExecution =
		std::chrono::steady_clock::now()
);
```

The second overload defaults to pool `0`.

---

### Basic Example

```cpp
auto task = Context::Schedule(
	std::chrono::milliseconds(1000),
	[]()
	{
		std::cout << "Running every second!" << std::endl;
	}
);
```

---

### Using a Specific Pool

```cpp
auto task = Context::Schedule(
	1,
	std::chrono::milliseconds(500),
	[]()
	{
		PollNetworkState();
	}
);
```

---

### Delayed First Execution

```cpp
auto task = Context::Schedule(
	std::chrono::milliseconds(1000),
	[]()
	{
		Heartbeat();
	},
	std::chrono::steady_clock::now() + std::chrono::milliseconds(2000)
);
```

---

### Cancelling a Scheduled Task

```cpp
task->Cancel();
```

---

## Thread Pools

Clockwork++ supports multiple isolated thread pools.

```cpp
Context::Create(3, 4);
```

Example layout:

| Pool | Purpose         |
| ---- | --------------- |
| 0    | General Tasks   |
| 1    | Networking      |
| 2    | Asset Streaming |

Tasks are routed to the appropriate pool at dispatch time.

---

## Worker Threads

Each thread pool owns a set of worker threads managed automatically by the context.

```cpp
Context::Create(2, 4);
```

This creates:

* Pool 0 → 4 threads
* Pool 1 → 4 threads

Total: 8 worker threads.

Workers continuously process queued tasks by priority until the context is destroyed.

---

## Example

```cpp
using namespace Raidcore::Clockwork;

int main()
{
	Context::Create(2, 4);

	auto task = Run<int>(
		0,
		ETaskPriority::Normal,
		[]()
		{
			return 123;
		}
	);

	auto scheduled = Context::Schedule(
		std::chrono::milliseconds(1000),
		[]()
		{
			Tick();
		}
	);

	int value = task->Await();

	Context::Destroy();

	return 0;
}
```

---

## Credits
Developed by [Kevin Bieniek](https://github.com/DeltaGW2) as part of the [Nexus](https://github.com/RaidcoreGG/Nexus) Engine project for [Raidcore](https://raidcore.gg).

Inspiration for the library was drawn from [ArenaNet](https://arena.net/)'s Arch Engine and the general implementation of async in C#.

---

## License
Clockwork++ is licensed under the MIT license. See [LICENSE](LICENSE) for details.
