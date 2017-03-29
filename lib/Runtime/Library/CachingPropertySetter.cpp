#include "RuntimeLibraryPch.h"
#include "CachingPropertySetter.h"

namespace Js
{
    struct JsonTypeCache
    {
        const Js::PropertyRecord* propertyRecord;
        Js::DynamicType* typeWithoutProperty;
        Js::DynamicType* typeWithProperty;
        JsonTypeCache* next;
        Js::PropertyIndex propertyIndex;

        JsonTypeCache(const Js::PropertyRecord* propertyRecord, Js::DynamicType* typeWithoutProperty, Js::DynamicType* typeWithProperty, Js::PropertyIndex propertyIndex) :
            propertyRecord(propertyRecord),
            typeWithoutProperty(typeWithoutProperty),
            typeWithProperty(typeWithProperty),
            propertyIndex(propertyIndex),
            next(nullptr) {}

        static JsonTypeCache* New(ArenaAllocator* allocator,
            const Js::PropertyRecord* propertyRecord,
            Js::DynamicType* typeWithoutProperty,
            Js::DynamicType* typeWithProperty,
            Js::PropertyIndex propertyIndex)
        {
            return Anew(allocator, JsonTypeCache, propertyRecord, typeWithoutProperty, typeWithProperty, propertyIndex);
        }

        void Update(const Js::PropertyRecord* propertyRecord,
            Js::DynamicType* typeWithoutProperty,
            Js::DynamicType* typeWithProperty,
            Js::PropertyIndex propertyIndex)
        {
            this->propertyRecord = propertyRecord;
            this->typeWithoutProperty = typeWithoutProperty;
            this->typeWithProperty = typeWithProperty;
            this->propertyIndex = propertyIndex;
        }
    };

	void CachingPropertySetter::SetProperty(Js::DynamicObject *object, Js::Var value, const WCHAR* currentStr, uint currentStrLength, bool useCache)
	{
        Js::DynamicType* typeWithoutProperty = object->GetDynamicType();
		if(useCache)
		{
			if (!previousCache)
			{
				// This is the first property in the list - see if we have an existing cache for it.
				currentCache = typeCacheList->LookupWithKey(Js::HashedCharacterBuffer<WCHAR>(currentStr, currentStrLength), nullptr);
			}
			if (currentCache && currentCache->typeWithoutProperty == typeWithoutProperty &&
				currentCache->propertyRecord->Equals(JsUtil::CharacterBuffer<WCHAR>(currentStr, currentStrLength)))
			{
				// Cache all values from currentCache as there is a chance that ParseObject might change the cache
				Js::DynamicType* typeWithProperty = currentCache->typeWithProperty;
				Js::PropertyId propertyId = currentCache->propertyRecord->GetPropertyId();
				Js::PropertyIndex propertyIndex = currentCache->propertyIndex;
				previousCache = currentCache;
				currentCache = currentCache->next;

				// fast path for type transition and property set
				object->EnsureSlots(typeWithoutProperty->GetTypeHandler()->GetSlotCapacity(),
					typeWithProperty->GetTypeHandler()->GetSlotCapacity(), scriptContext, typeWithProperty->GetTypeHandler());

				object->ReplaceType(typeWithProperty);
				object->SetSlot(SetSlotArguments(propertyId, propertyIndex, value));
				return;
			}
		}
		// slow path
		Js::PropertyRecord const * propertyRecord;
		scriptContext->GetOrAddPropertyRecord(currentStr, currentStrLength, &propertyRecord);

		Js::PropertyValueInfo info;
		object->SetProperty(propertyRecord->GetPropertyId(), value, Js::PropertyOperation_None, &info);

		if (!useCache)
			return;

		Js::DynamicType* typeWithProperty = object->GetDynamicType();
		if (!propertyRecord->IsNumeric() && !info.IsNoCache() && typeWithProperty->GetIsShared() && typeWithProperty->GetTypeHandler()->IsPathTypeHandler())
		{
			Js::PropertyIndex propertyIndex = info.GetPropertyIndex();

			if (!previousCache)
			{
				// This is the first property in the set add it to the dictionary.
				currentCache = JsonTypeCache::New(arenaAllocator, propertyRecord, typeWithoutProperty, typeWithProperty, propertyIndex);
				typeCacheList->AddNew(propertyRecord, currentCache);
			}
			else if (!currentCache)
			{
				currentCache = JsonTypeCache::New(arenaAllocator, propertyRecord, typeWithoutProperty, typeWithProperty, propertyIndex);
				previousCache->next = currentCache;
			}
			else
			{
				// cache miss!!
				currentCache->Update(propertyRecord, typeWithoutProperty, typeWithProperty, propertyIndex);
			}
			previousCache = currentCache;
			currentCache = currentCache->next;
		}
		
		return;
	}
}