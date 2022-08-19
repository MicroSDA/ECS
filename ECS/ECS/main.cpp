
#include "ECS.h"
#include <iostream>

using namespace ecs;


class Component
{
public:
	using type = int;
	Component() { printf_s("Component created!\n"); };
	virtual ~Component(){ printf_s("~Component destroyed!\n"); };
private:
	
};

int main()
{
	EntityManager manager;
	manager.SetOnEntityCreate([](Entity& entity) { entity.AddComponent<std::string>("Entity"); });

	Entity entity1 = manager.CreateEntity();
	Entity entity2 = manager.CreateEntity();
	Entity entity3 = manager.CreateEntity();
	Entity entity4 = manager.CreateEntity();


	/*printf_s("Alive entities counst: %d\n", manager.EntitiesCount());

	for (auto& entity : manager)
		printf_s("Entity id: %d\n", entity);*/

	/* Add component */
	entity1.AddComponent<Component>();
	entity2.AddComponent<Component>();
	entity3.AddComponent<Component>();
	entity4.AddComponent<Component>();

	entity1.AddComponent<float>();
	entity2.AddComponent<float>();
	entity3.AddComponent<float>();

	entity1.AddComponent<char>();
	entity2.AddComponent<char>();

	/* Link entities with each other !*/
	entity1.AddChild(entity2);
	
	entity1.RemoveChild(entity2);


	

	/*entity1.AddChild(entity3);
	entity1.AddChild(entity4);*/

	/* Get entities with given components */
	//manager.View<Component> ().Each([](Entity entity, Component& component)
	//	{
	//		printf_s("Entity with Component id: %d\n", entity.GetID());
	//	});
	///* Get entities with given components */
	//manager.View<Component, float>().Each([](Entity entity, Component& component, float& _float)
	//	{
	//		printf_s("Entity with Component & flaot id: %d\n", entity.GetID());
	//	});
	//manager.View<Component, float, char>().Each([](Entity entity, Component& component, float& _float, char& _char)
	//	{
	//		printf_s("Entity with Component & flaot & char id: %d\n", entity.GetID());
	//	});

	manager.DestroyAllEntites();
	//entity1.DestroyWithChildren();
	return 0;
}