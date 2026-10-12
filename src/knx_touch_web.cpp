#include "knx_touch_web.h"

#include <cstdlib>
#include <cstring>

namespace {
void htmlEscape(const char* input, char* output, size_t capacity) {
  if (!output || capacity == 0) return;
  size_t used = 0;
  for (const char* p = input ? input : ""; *p && used + 1 < capacity; ++p) {
    const char* replacement = nullptr;
    switch (*p) {
      case '&': replacement = "&amp;"; break;
      case '<': replacement = "&lt;"; break;
      case '>': replacement = "&gt;"; break;
      case '\"': replacement = "&quot;"; break;
      case '\'': replacement = "&#39;"; break;
      default: output[used++] = *p; continue;
    }
    const size_t length = strlen(replacement);
    if (used + length >= capacity) break;
    memcpy(output + used, replacement, length);
    used += length;
  }
  output[used] = '\0';
}

bool parseUnsigned(const String& text, uint32_t minimum, uint32_t maximum,
                   uint32_t& value) {
  if (text.length() == 0) return false;
  char* end = nullptr;
  const unsigned long parsed = strtoul(text.c_str(), &end, 10);
  if (!end || *end != '\0' || parsed < minimum || parsed > maximum) return false;
  value = static_cast<uint32_t>(parsed);
  return true;
}
}

bool KnxTouchWeb::registerRoutes(ESPBase& espBase, KnxTouchConfigStore& store,
                                 ChangeHandler handler, void* context,
                                 ModeHandler modeHandler, ModeReader modeReader) {
  store_ = &store;
  changeHandler_ = handler;
  changeContext_ = context;
  modeHandler_ = modeHandler;
  modeReader_ = modeReader;
  return espBase.addPage("KNX / Touches", "/knx-touches", handlePage, this) &&
         espBase.addPostRoute("/knx-touches/save", handleSave, this) &&
         espBase.addPostRoute("/knx-touches/active", handleActive, this) &&
         espBase.addPostRoute("/knx-touches/passive", handlePassive, this);
}

void KnxTouchWeb::handlePage(WebRequest&, WebResponse& response, void* context) {
  static_cast<KnxTouchWeb*>(context)->render(response, nullptr, false);
}

void KnxTouchWeb::handleSave(WebRequest& request, WebResponse& response, void* context) {
  auto* self = static_cast<KnxTouchWeb*>(context);
  KnxTouchConfigData candidate = self->store_->data();
  if (!request.hasArg("individual_address") ||
      !KnxTouchConfigStore::parseIndividualAddress(
          request.arg("individual_address").c_str(),
          candidate.general.individualAddress)) {
    self->render(response, "Adresse individuelle invalide (format 0..15.0..15.1..255).",
                 true, &request, "individual_address");
    return;
  }
  if (!request.hasArg("router_ip") ||
      !KnxTouchConfigStore::parseIpv4(request.arg("router_ip").c_str(),
                                      candidate.general.routerIpv4, true)) {
    self->render(response, "Adresse IPv4 du routeur invalide.", true, &request, "router_ip");
    return;
  }
  if (!request.hasArg("multicast_ip") ||
      !KnxTouchConfigStore::parseIpv4(request.arg("multicast_ip").c_str(),
                                      candidate.general.multicastIpv4)) {
    self->render(response, "Adresse IPv4 multicast invalide.", true, &request, "multicast_ip");
    return;
  }
  uint32_t parsed = 0;
  if (!request.hasArg("knx_port") ||
      !parseUnsigned(request.arg("knx_port"), 1, 65535, parsed)) {
    self->render(response, "Port KNXnet/IP invalide (1..65535).", true, &request, "knx_port");
    return;
  }
  candidate.general.port = static_cast<uint16_t>(parsed);
  if (!request.hasArg("feedback_timeout_ms") ||
      !parseUnsigned(request.arg("feedback_timeout_ms"), 1000, 60000, parsed)) {
    self->render(response, "Timeout invalide (1000..60000 ms).", true, &request,
                 "feedback_timeout_ms");
    return;
  }
  candidate.general.feedbackTimeoutMs = parsed;
  candidate.general.transport = KnxConfiguredTransport::Routing;
  candidate.general.synchronization = KnxSynchronizationMode::Passive;
  candidate.general.reserved = 0;
  char field[16]{};
  for (uint8_t index = 0; index < 4; ++index) {
    snprintf(field, sizeof(field), "name%u", index);
    if (!request.hasArg(field)) {
      self->render(response, "Champ nom manquant.", true, &request, field);
      return;
    }
    const String name = request.arg(field);
    if (name.length() == 0 || name.length() >= KnxTouchButtonConfig::kNameCapacity) {
      self->render(response, "Nom vide ou trop long (23 caracteres maximum).", true,
                   &request, field);
      return;
    }
    strlcpy(candidate.buttons[index].name, name.c_str(),
            sizeof(candidate.buttons[index].name));

    snprintf(field, sizeof(field), "command%u", index);
    if (!request.hasArg(field) || !KnxTouchConfigStore::parseGroupAddress(
                                      request.arg(field).c_str(),
                                      candidate.buttons[index].commandAddress)) {
      self->render(response, "Adresse de commande invalide (format 0..31/0..7/0..255).",
                   true, &request, field);
      return;
    }
    snprintf(field, sizeof(field), "status%u", index);
    if (!request.hasArg(field) || !KnxTouchConfigStore::parseGroupAddress(
                                      request.arg(field).c_str(),
                                      candidate.buttons[index].statusAddress)) {
      self->render(response, "Adresse de statut invalide (format 0..31/0..7/0..255).",
                   true, &request, field);
      return;
    }
    snprintf(field, sizeof(field), "enabled%u", index);
    candidate.buttons[index].enabled = request.hasArg(field) ? 1 : 0;
  }
  KnxTouchConfigStore::finalize(candidate);
  if (!KnxTouchConfigStore::validate(candidate)) {
    self->render(response,
                 "Configuration refusee: une touche utilise la meme adresse en commande et statut.",
                 true, &request);
    return;
  }
  const bool changed = memcmp(&candidate, &self->store_->data(), sizeof(candidate)) != 0;
  if (!self->store_->save(candidate)) {
    self->render(response, "Echec de l'enregistrement persistant.", true);
    return;
  }
  if (changed && self->changeHandler_) self->changeHandler_(self->changeContext_);
  self->render(response, changed ? "Configuration enregistree." : "Configuration inchangee; aucune ecriture flash.", false);
}

void KnxTouchWeb::handleActive(WebRequest&, WebResponse& response, void* context) {
  auto* self = static_cast<KnxTouchWeb*>(context);
  const bool accepted = self->modeHandler_ &&
                        self->modeHandler_(true, self->changeContext_);
  self->render(response,
               accepted ? "Mode ACTIVE active et enregistre."
                        : "Activation refusee par les protections KNX ou reseau.",
               !accepted);
}

void KnxTouchWeb::handlePassive(WebRequest&, WebResponse& response, void* context) {
  auto* self = static_cast<KnxTouchWeb*>(context);
  const bool accepted = self->modeHandler_ &&
                        self->modeHandler_(false, self->changeContext_);
  self->render(response, accepted ? "Mode PASSIVE actif; emissions bloquees."
                                  : "Impossible de confirmer le mode PASSIVE.",
               !accepted);
}

void KnxTouchWeb::render(WebResponse& response, const char* message, bool error,
                         WebRequest* submitted, const char* errorField) {
  response.beginPage("KNX / Touches");
  const bool active = modeReader_ && modeReader_(changeContext_);
  response.write("<style>"
                 ".knx-form{display:grid;gap:.55rem}.knx-form fieldset{margin:0;padding:.55rem .75rem}"
                 ".knx-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(210px,1fr));gap:.45rem .75rem}"
                 ".knx-grid label{margin:0}.knx-button{border-left:5px solid #9e9e9e}"
                 ".knx-button.enabled{border-left-color:#16833b;background:#f2fff5}"
                 ".knx-check{display:flex;align-items:center;gap:.5rem}.knx-check input{width:auto;margin:0}"
                 ".knx-error{outline:2px solid #b00020;background:#fff4f4}"
                 "</style><section class='card'>");
  response.write(active ? "<p><strong>Mode KNX : ACTIVE</strong></p>"
                          "<p>Seuls les appuis tactiles valides peuvent emettre un GroupValueWrite.</p>"
                        : "<p><strong>Mode KNX : PASSIVE</strong></p>"
                          "<p>Aucun GroupValueRead ni GroupValueWrite n'est emis.</p>");
  response.write(active
      ? "<form method='post' action='/knx-touches/passive'><button type='submit'>Revenir immediatement en PASSIVE</button></form>"
      : "<form method='post' action='/knx-touches/active'><button type='submit'>Activer volontairement ACTIVE</button></form>"
        "<p>Le mode choisi est conserve apres redemarrage.</p>");
  if (message) {
    response.write(error ? "<p style='color:#b00020'><strong>Erreur : " :
                           "<p style='color:#087f23'><strong>");
    response.write(message);
    response.write("</strong></p>");
  }
  response.write("</section><section class='card'><form class='knx-form' method='post' action='/knx-touches/save'>");
  const auto& general = store_->data().general;
  char individual[16]{};
  char router[20]{};
  char multicast[20]{};
  char html[192]{};
  KnxTouchConfigStore::formatIndividualAddress(general.individualAddress, individual,
                                                sizeof(individual));
  KnxTouchConfigStore::formatIpv4(general.routerIpv4, router, sizeof(router));
  KnxTouchConfigStore::formatIpv4(general.multicastIpv4, multicast, sizeof(multicast));
  auto valueFor = [submitted](const char* field, const char* fallback,
                              char* output, size_t capacity) {
    const char* value = fallback;
    String submittedValue;
    if (submitted && submitted->hasArg(field)) {
      submittedValue = submitted->arg(field);
      value = submittedValue.c_str();
    }
    htmlEscape(value, output, capacity);
  };
  char escaped[128]{};
  response.write("<fieldset><legend>KNX / General</legend>"
                 "<p><strong>Transport :</strong> Routing uniquement</p>"
                 "<p><strong>Synchronisation initiale :</strong> passive, sans GroupValueRead</p>"
                 "<p><strong>Mode au demarrage :</strong> dernier choix ACTIVE/PASSIVE enregistre</p>"
                 "<p>Les parametres reseau KNX sont appliques au prochain redemarrage.</p>");
  response.write("<div class='knx-grid'>");
  valueFor("individual_address", individual, escaped, sizeof(escaped));
  snprintf(html, sizeof(html),
           "<label>Adresse individuelle<input class='%s' name='individual_address' required "
           "pattern='[0-9]{1,2}\\.[0-9]{1,2}\\.[0-9]{1,3}' value='%s'></label>",
           errorField && strcmp(errorField, "individual_address") == 0 ? "knx-error" : "", escaped);
  response.write(html);
  valueFor("router_ip", router, escaped, sizeof(escaped));
  snprintf(html, sizeof(html),
           "<label>Routeur KNX/IP (facultatif)<input class='%s' name='router_ip' value='%s'></label>",
           errorField && strcmp(errorField, "router_ip") == 0 ? "knx-error" : "", escaped);
  response.write(html);
  char fallback[24]{};
  snprintf(fallback, sizeof(fallback), "%u", general.port);
  valueFor("knx_port", fallback, escaped, sizeof(escaped));
  snprintf(html, sizeof(html),
           "<label>Port KNXnet/IP<input class='%s' name='knx_port' type='number' min='1' "
           "max='65535' required value='%s'></label>",
           errorField && strcmp(errorField, "knx_port") == 0 ? "knx-error" : "", escaped);
  response.write(html);
  valueFor("multicast_ip", multicast, escaped, sizeof(escaped));
  snprintf(html, sizeof(html),
           "<label>Multicast Routing<input class='%s' name='multicast_ip' required value='%s'></label>",
           errorField && strcmp(errorField, "multicast_ip") == 0 ? "knx-error" : "", escaped);
  response.write(html);
  snprintf(fallback, sizeof(fallback), "%lu", static_cast<unsigned long>(general.feedbackTimeoutMs));
  valueFor("feedback_timeout_ms", fallback, escaped, sizeof(escaped));
  snprintf(html, sizeof(html),
           "<label>Timeout retour (ms)<input class='%s' name='feedback_timeout_ms' "
           "type='number' min='1000' max='60000' required value='%s'></label></div></fieldset>",
           errorField && strcmp(errorField, "feedback_timeout_ms") == 0 ? "knx-error" : "", escaped);
  response.write(html);
  for (uint8_t index = 0; index < 4; ++index) {
    const auto& button = store_->data().buttons[index];
    char escapedName[128]{};
    char command[16]{};
    char status[16]{};
    char html[192]{};
    KnxTouchConfigStore::formatGroupAddress(button.commandAddress, command, sizeof(command));
    KnxTouchConfigStore::formatGroupAddress(button.statusAddress, status, sizeof(status));
    char nameField[16]{}, commandField[16]{}, statusField[16]{}, enabledField[16]{};
    snprintf(nameField, sizeof(nameField), "name%u", index);
    snprintf(commandField, sizeof(commandField), "command%u", index);
    snprintf(statusField, sizeof(statusField), "status%u", index);
    snprintf(enabledField, sizeof(enabledField), "enabled%u", index);
    const bool enabled = submitted ? submitted->hasArg(enabledField) : button.enabled;
    valueFor(nameField, button.name, escapedName, sizeof(escapedName));
    snprintf(html, sizeof(html), "<fieldset class='knx-button %s'><legend>Touche %u</legend><div class='knx-grid'><label>Nom<input class='%s' name='name%u' maxlength='23' required value='",
             enabled ? "enabled" : "disabled", index + 1,
             errorField && strcmp(errorField, nameField) == 0 ? "knx-error" : "", index);
    response.write(html);
    response.write(escapedName);
    snprintf(html, sizeof(html),
             "'></label><label>Groupe commande<input class='%s' name='command%u' required "
             "pattern='[0-9]{1,2}/[0-7]/[0-9]{1,3}' value='%s'></label>",
             errorField && strcmp(errorField, commandField) == 0 ? "knx-error" : "", index,
             (valueFor(commandField, command, escaped, sizeof(escaped)), escaped));
    response.write(html);
    snprintf(html, sizeof(html),
             "<label>Groupe statut<input class='%s' name='status%u' required "
             "pattern='[0-9]{1,2}/[0-7]/[0-9]{1,3}' value='%s'></label>",
             errorField && strcmp(errorField, statusField) == 0 ? "knx-error" : "", index,
             (valueFor(statusField, status, escaped, sizeof(escaped)), escaped));
    response.write(html);
    snprintf(html, sizeof(html),
             "</div><label class='knx-check'><input type='checkbox' name='enabled%u' value='1'%s> "
             "Touche active</label></fieldset>",
             index, enabled ? " checked" : "");
    response.write(html);
  }
  response.write("<button type='submit'>Enregistrer</button></form></section>");
  response.endPage();
}
