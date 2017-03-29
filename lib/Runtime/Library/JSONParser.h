//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once
#include "JSONScanner.h"
#include "CachingPropertySetter.h"

namespace JSON
{
    class JSONDeferredParserRootNode;

    class JSONParser
    {
    public:
        JSONParser(Js::ScriptContext* sc, Js::RecyclableObject* rv) : scriptContext(sc),
            reviver(rv), propertySetter(nullptr)
        {
            this->propertySetter = new Js::CachingPropertySetter(scriptContext);
        };

        Js::Var Parse(LPCWSTR str, uint length);
        Js::Var Parse(Js::JavascriptString* input);
        Js::Var Walk(Js::JavascriptString* name, Js::PropertyId id, Js::Var holder, uint32 index = Js::JavascriptArray::InvalidIndex);
        void Finalizer();

    private:
        tokens Scan()
        {
            return m_scanner.Scan();
        }

        Js::Var ParseObject();

        void CheckCurrentToken(int tk, int wErr)
        {
            if (m_token.tk != tk)
                m_scanner.ThrowSyntaxError(wErr);
            Scan();
        }

        Token m_token;
        JSONScanner m_scanner;
        Js::ScriptContext* scriptContext;
        Js::RecyclableObject* reviver;

        bool isCaching;
        Js::CachingPropertySetter* propertySetter;

        static const uint MIN_CACHE_LENGTH = 50; // Use Json type cache only if the JSON string is larger than this constant.
    };
} // namespace JSON
