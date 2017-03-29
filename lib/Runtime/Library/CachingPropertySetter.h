#pragma once

namespace Js
{
    struct JsonTypeCache;
	
	class CachingPropertySetter
	{
	public:
		CachingPropertySetter(Js::ScriptContext* sc) : scriptContext(sc),
            arenaAllocator(nullptr), arenaAllocatorObject(nullptr), typeCacheList(nullptr)
		{
			this->arenaAllocatorObject = scriptContext->GetTemporaryGuestAllocator(_u("CachingPropertySetter"));
			this->arenaAllocator = arenaAllocatorObject->GetAllocator();
			this->typeCacheList = Anew(this->arenaAllocator, JsonTypeCacheList, this->arenaAllocator, 8);
		};

		void Finalizer()
		{
			if (arenaAllocatorObject)
			{
				this->scriptContext->ReleaseTemporaryGuestAllocator(arenaAllocatorObject);
				arenaAllocator = nullptr;
				arenaAllocatorObject = nullptr;
			}
		}

		void SetProperty(Js::DynamicObject *object, Js::Var value, const WCHAR* currentStr, uint currentStrLength, bool useCache);

		ArenaAllocator* GetArenaAllocator() 
		{
			return this->arenaAllocator;
		}

	private:
		Js::ScriptContext *scriptContext;

		JsonTypeCache* previousCache = nullptr;
		JsonTypeCache* currentCache = nullptr;

		Js::TempGuestArenaAllocatorObject* arenaAllocatorObject;
		ArenaAllocator* arenaAllocator;
		typedef JsUtil::BaseDictionary<const Js::PropertyRecord *, JsonTypeCache*, ArenaAllocator, PowerOf2SizePolicy, Js::PropertyRecordStringHashComparer>  JsonTypeCacheList;

		JsonTypeCacheList *typeCacheList = nullptr;
	};
}
