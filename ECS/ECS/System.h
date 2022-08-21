#pragma once
#include <unordered_map>

namespace ecs
{
	class BasicSystem
	{
	public:
		BasicSystem() = default;
		virtual ~BasicSystem() = default;
	};

	template<typename Component>
	class System : public BasicSystem
	{
	public:
		System(void(*onCreate)(Component&), void(*onUpdate)(Component&), void(*onDestroy)(Component&)):
			OnCreate(onCreate), OnUpdate(onUpdate), OnDestroy(onDestroy) {}
		virtual ~System() = default;
		void(*OnCreate)(Component&) = nullptr;
		void(*OnUpdate)(Component&) = nullptr;
		void(*OnDestroy)(Component&) = nullptr;
	};
}