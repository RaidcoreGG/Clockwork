# Clockwork++

A lightweight asynchronous task scheduling library designed for high-performance applications and game engines.

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
* Minimal dependencies
* Designed for engine and real-time application workloads

---

## Getting Started

### Creating the Context

Before dispatching any work, a Clockwork context must be created.

```cpp
using namespace Raidcore::Clockwork;

// Create 4 thread pools with automatic thread counts.
Context::Create(4);

// Or specify both pool count and threads per pool.
Context::Create(4, 2);
```

The context manages all worker threads and task queues.

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

// Wait for completion.
int result = task->Await();
```

Tasks may also be dispatched directly to a specific thread pool.

```cpp
auto task = Run<std::string>(
	1, // Pool ID
	ETaskPriority::High,
	[]()
	{
		return "Hello Clockwork";
	}
);
```

---

## Task Priorities

Clockwork++ supports prioritized execution.

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

Higher-priority queues are processed before lower-priority queues within a thread pool.

Typical usage:

| Priority  | Purpose                      |
| --------- | ---------------------------- |
| Immediate | Time-sensitive engine work   |
| High      | Networking, gameplay systems |
| Normal    | General application tasks    |
| Low       | Background processing        |

---

## Dispatchers

A `Dispatcher<T>` stores a reusable asynchronous operation together with its target thread pool and priority.

This is useful when the same task needs to be scheduled repeatedly. For example workers.

### Creating a Dispatcher

```cpp
Dispatcher<int> dispatcher(
	[]()
	{
		return PerformExpensiveCalculation();
	},
	2,                        // Pool
	ETaskPriority::High       // Priority
);
```

### Dispatching Work

```cpp
auto task = dispatcher.Dispatch();

int result = task->Await();
```

The dispatcher can also be invoked directly.

```cpp
auto task = dispatcher();
```

### Reconfiguring a Dispatcher

```cpp
dispatcher.SetPool(1);

dispatcher.SetPriority(
	ETaskPriority::Critical
);

dispatcher.SetAction(
	[]()
	{
		return GenerateData();
	}
);
```

This makes dispatchers ideal for event handlers, recurring engine jobs, and reusable work definitions.

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

```cpp
Run<void>(
	1,
	ETaskPriority::High,
	[]()
	{
		ReceivePackets();
	}
);

Run<void>(
	2,
	ETaskPriority::Normal,
	[]()
	{
		LoadTexture();
	}
);
```

This prevents expensive workloads from starving latency-sensitive systems.

---

## Worker Threads

Each thread pool owns a collection of worker threads.

Workers continuously process queued tasks according to priority until the Clockwork context is destroyed.

Thread management is fully automatic after initialization.

```cpp
Context::Create(2, 4);
```

Creates:

* Pool 0 with 4 worker threads
* Pool 1 with 4 worker threads

For a total of 8 workers.

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
Clockwork++ is a proprietary codebase. All rights reserved.
