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

#include "config.h"

#include "PdfPluginReplacement.h"
#include "Event.h"
#include "HTMLDivElement.h"
#include "HTMLPlugInElement.h"
#include "JSDOMBinding.h"
#include "JSDOMGlobalObject.h"
#include "Logging.h"
#include "MainFrame.h"
#include "Page.h"
#include "RenderElement.h"
#include "ScriptController.h"
#include "ScriptSourceCode.h"
#include "UserAgentScripts.h"
//#include <objc/runtime.h>
//#include <JavaScriptCore/JavaScriptCore.h>
#include "JSHTMLDivElement.h"
#include <JavaScriptCore/APICast.h>
#include <wtf/text/Base64.h>

namespace WebCore {

static String pdfPluginReplacementScript()
{
    static NeverDestroyed<String> script(PdfPluginReplacementJavaScript, sizeof(PdfPluginReplacementJavaScript));
    return script;
}

void PdfPluginReplacement::registerPluginReplacement(PluginReplacementRegistrar registrar)
{
    registrar(ReplacementPlugin(create, supportsMimeType, supportsFileExtension, supportsURL));
}

PassRefPtr<PluginReplacement> PdfPluginReplacement::create(HTMLPlugInElement& plugin, const Vector<String>& paramNames, const Vector<String>& paramValues)
{
    return adoptRef(new PdfPluginReplacement(plugin, paramNames, paramValues));
}

bool PdfPluginReplacement::supportsMimeType(const String& mimeType)
{
    return equalLettersIgnoringASCIICase(mimeType, "application/pdf") ||
           equalLettersIgnoringASCIICase(mimeType, "application/x-pdf") ||
           equalLettersIgnoringASCIICase(mimeType, "text/pdf");
}

bool PdfPluginReplacement::supportsFileExtension(const String& extension)
{
    return equalLettersIgnoringASCIICase(extension, "pdf");
}

PdfPluginReplacement::PdfPluginReplacement(HTMLPlugInElement& plugin, const Vector<String>& paramNames, const Vector<String>& paramValues)
    : PluginReplacement()
    , m_parentElement(&plugin)
    , m_names(paramNames)
    , m_values(paramValues)
{

}

RenderPtr<RenderElement> PdfPluginReplacement::createElementRenderer(HTMLPlugInElement& plugin, Ref<RenderStyle>&& style, const RenderTreePosition& insertionPosition)
{
  ASSERT_UNUSED(plugin, m_parentElement == &plugin);

  LOG(Plugins, "%p - createElementRenderer: %p", this, m_pdfElement);

  if (m_pdfElement)
      return m_pdfElement->createElementRenderer(WTFMove(style), insertionPosition);

  return nullptr;
}

DOMWrapperWorld& PdfPluginReplacement::isolatedWorld()
{
    static DOMWrapperWorld& isolatedWorld = DOMWrapperWorld::create(JSDOMWindow::commonVM()).leakRef();
    return isolatedWorld;
}

bool PdfPluginReplacement::ensureReplacementScriptInjected()
{
    if (!m_parentElement->document().frame())
        return false;

    DOMWrapperWorld& world = isolatedWorld();
    ScriptController& scriptController = m_parentElement->document().frame()->script();
    JSDOMGlobalObject* globalObject = JSC::jsCast<JSDOMGlobalObject*>(scriptController.globalObject(world));
    JSC::ExecState* exec = globalObject->globalExec();
    JSC::JSLockHolder lock(exec);

    JSC::JSValue replacementFunction = globalObject->get(exec, JSC::Identifier::fromString(exec, "createPluginReplacement"));
    if (replacementFunction.isFunction())
        return true;

    scriptController.evaluateInWorld(ScriptSourceCode(pdfPluginReplacementScript()), world);
    if (exec->hadException()) {
        LOG(Plugins, "%p - Exception when evaluating Pdf plugin replacement script", this);
        exec->clearException();
        return false;
    }

    return true;
}

bool PdfPluginReplacement::installReplacement(ShadowRoot* root)
{
  if (!ensureReplacementScriptInjected())
      return false;

  if (!m_parentElement->document().frame())
      return false;

  DOMWrapperWorld& world = isolatedWorld();
  ScriptController& scriptController = m_parentElement->document().frame()->script();
  JSDOMGlobalObject* globalObject = JSC::jsCast<JSDOMGlobalObject*>(scriptController.globalObject(world));
  JSC::ExecState* exec = globalObject->globalExec();
  JSC::JSLockHolder lock(exec);

  // Lookup the "createPluginReplacement" function.
  JSC::JSValue replacementFunction = globalObject->get(exec, JSC::Identifier::fromString(exec, "createPluginReplacement"));
  if (replacementFunction.isUndefinedOrNull())
      return false;
  JSC::JSObject* replacementObject = replacementFunction.toObject(exec);
  JSC::CallData callData;
  JSC::CallType callType = replacementObject->methodTable()->getCallData(replacementObject, callData);
  if (callType == JSC::CallTypeNone)
      return false;

  JSC::MarkedArgumentBuffer argList;
  argList.append(toJS(exec, globalObject, root));
  argList.append(toJS(exec, globalObject, m_parentElement));
  argList.append(toJS<String>(exec, globalObject, m_names));
  argList.append(toJS<String>(exec, globalObject, m_values));
  JSC::JSValue replacement = call(exec, replacementObject, callType, callData, globalObject, argList);
  if (exec->hadException()) {
      exec->clearException();
      LOG(Plugins, "%p - installReplacement exception", this);
      return false;
  }

  // Get the <pdf> created to replace the plug-in.
  JSC::JSValue value = replacement.get(exec, JSC::Identifier::fromString(exec, "pdf"));
  if (!exec->hadException() && !value.isUndefinedOrNull())
      m_pdfElement = (HTMLDivElement *)JSHTMLDivElement::toWrapped(value);

  if (!m_pdfElement) {
      LOG(Plugins, "%p - Failed to find <pdf> element created by Pdf plugin replacement script.", this);
      exec->clearException();
      return false;
  }

  return true;
}

bool PdfPluginReplacement::supportsURL(const URL& url)
{
    return true;
}

}
