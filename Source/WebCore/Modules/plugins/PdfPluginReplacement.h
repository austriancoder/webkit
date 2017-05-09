/*
 * Copyright (C) 2017 Bachmann electronic GmbH. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PdfPluginReplacement_h
#define PdfPluginReplacement_h

#include "PluginReplacement.h"
#include "ScriptState.h"
#include <bindings/ScriptObject.h>
#include <wtf/RefCounted.h>


namespace WebCore {

class HTMLPlugInElement;
class HTMLDivElement;
class RenderElement;
class RenderStyle;
class ShadowRoot;
class PdfEmbedShadowElement;

class PdfPluginReplacement : public PluginReplacement {
public:
    static void registerPluginReplacement(PluginReplacementRegistrar);

private:
    PdfPluginReplacement(HTMLPlugInElement&, const Vector<String>& paramNames, const Vector<String>& paramValues);

    static bool supportsMimeType(const String&);
    static bool supportsFileExtension(const String&);
    static bool supportsURL(const URL&);

    static PassRefPtr<PluginReplacement> create(HTMLPlugInElement&, const Vector<String>& paramNames, const Vector<String>& paramValues);

    virtual bool installReplacement(ShadowRoot*) override;

    virtual bool willCreateRenderer() override { return m_pdfElement; }
    virtual RenderPtr<RenderElement> createElementRenderer(HTMLPlugInElement&, Ref<RenderStyle>&&, const RenderTreePosition&) override;

    bool ensureReplacementScriptInjected();
    DOMWrapperWorld& isolatedWorld();

    HTMLPlugInElement* m_parentElement;
    HTMLDivElement* m_pdfElement;
    const Vector<String> m_names;
    const Vector<String> m_values;
};

}

#endif
