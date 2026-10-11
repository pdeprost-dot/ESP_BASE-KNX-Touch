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
                                 ChangeHandler handler, void* context) {
  store_ = &store;
  changeHandler_ = handler;
  changeContext_ = context;
  return espBase.addPage("KNX / Touches", "/knx-touches", handlePage, this) &&
         espBase.addPostRoute("/knx-touches/save", handleSave, this);
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
    self->render(response, "Adresse individuelle invalide (format 0..15.0..15.1..255).", true);
    return;
  }
  if (!request.hasArg("router_ip") ||
      !KnxTouchConfigStore::parseIpv4(request.arg("router_ip").c_str(),
                                      candidate.general.routerIpv4, true)) {
    self->render(response, "Adresse IPv4 du routeur invalide.", true);
    return;
  }
  if (!request.hasArg("multicast_ip") ||
      !KnxTouchConfigStore::parseIpv4(request.arg("multicast_ip").c_str(),
                                      candidate.general.multicastIpv4)) {
    self->render(response, "Adresse IPv4 multicast invalide.", true);
    return;
  }
  uint32_t parsed = 0;
  if (!request.hasArg("knx_port") ||
      !parseUnsigned(request.arg("knx_port"), 1, 65535, parsed)) {
    self->render(response, "Port KNXnet/IP invalide (1..65535).", true);
    return;
  }
  candidate.general.port = static_cast<uint16_t>(parsed);
  if (!request.hasArg("feedback_timeout_ms") ||
      !parseUnsigned(request.arg("feedback_timeout_ms"), 1000, 60000, parsed)) {
    self->render(response, "Timeout invalide (1000..60000 ms).", true);
    return;
  }
  candidate.general.feedbackTimeoutMs = parsed;
  candidate.general.transport = KnxConfiguredTransport::Routing;
  candidate.general.synchronization = KnxSynchronizationMode::Passive;
  candidate.general.operatingMode = KnxOperatingMode::Passive;
  candidate.general.reserved = 0;
  char field[16]{};
  for (uint8_t index = 0; index < 4; ++index) {
    snprintf(field, sizeof(field), "name%u", index);
    if (!request.hasArg(field)) {
      self->render(response, "Champ nom manquant.", true);
      return;
    }
    const String name = request.arg(field);
    if (name.length() == 0 || name.length() >= KnxTouchButtonConfig::kNameCapacity) {
      self->render(response, "Nom vide ou trop long (23 caracteres maximum).", true);
      return;
    }
    strlcpy(candidate.buttons[index].name, name.c_str(),
            sizeof(candidate.buttons[index].name));

    snprintf(field, sizeof(field), "command%u", index);
    if (!request.hasArg(field) || !KnxTouchConfigStore::parseGroupAddress(
                                      request.arg(field).c_str(),
                                      candidate.buttons[index].commandAddress)) {
      self->render(response, "Adresse de commande invalide (format 0..31/0..7/0..255).", true);
      return;
    }
    snprintf(field, sizeof(field), "status%u", index);
    if (!request.hasArg(field) || !KnxTouchConfigStore::parseGroupAddress(
                                      request.arg(field).c_str(),
                                      candidate.buttons[index].statusAddress)) {
      self->render(response, "Adresse de statut invalide (format 0..31/0..7/0..255).", true);
      return;
    }
    snprintf(field, sizeof(field), "enabled%u", index);
    candidate.buttons[index].enabled = request.hasArg(field) ? 1 : 0;
  }
  KnxTouchConfigStore::finalize(candidate);
  if (!KnxTouchConfigStore::validate(candidate)) {
    self->render(response, "Configuration refusee: nom, adresse ou paire commande/statut invalide.", true);
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

void KnxTouchWeb::render(WebResponse& response, const char* message, bool error) {
  response.beginPage("KNX / Touches");
  response.write("<section class='card'><h2>KNX / Touches</h2>"
                 "<p><strong>Emission KNX : BLOQUEE</strong></p>"
                 "<p>Les retours d'etat restent passifs. Aucun GroupValueRead ni GroupValueWrite n'est emis.</p>");
  if (message) {
    response.write(error ? "<p style='color:#b00020'><strong>Erreur : " :
                           "<p style='color:#087f23'><strong>");
    response.write(message);
    response.write("</strong></p>");
  }
  response.write("</section><section class='card'><form method='post' action='/knx-touches/save'>");
  const auto& general = store_->data().general;
  char individual[16]{};
  char router[20]{};
  char multicast[20]{};
  char html[192]{};
  KnxTouchConfigStore::formatIndividualAddress(general.individualAddress, individual,
                                                sizeof(individual));
  KnxTouchConfigStore::formatIpv4(general.routerIpv4, router, sizeof(router));
  KnxTouchConfigStore::formatIpv4(general.multicastIpv4, multicast, sizeof(multicast));
  response.write("<fieldset><legend>KNX / General</legend>"
                 "<p><strong>Transport :</strong> Routing uniquement</p>"
                 "<p><strong>Synchronisation initiale :</strong> passive, sans GroupValueRead</p>"
                 "<p><strong>Mode d'exploitation :</strong> PASSIVE uniquement</p>"
                 "<p>Les parametres reseau KNX sont appliques au prochain redemarrage.</p>");
  snprintf(html, sizeof(html),
           "<label>Adresse individuelle<input name='individual_address' required "
           "pattern='[0-9]{1,2}\\.[0-9]{1,2}\\.[0-9]{1,3}' value='%s'></label>",
           individual);
  response.write(html);
  snprintf(html, sizeof(html),
           "<label>Routeur KNX/IP (facultatif en Routing)<input name='router_ip' "
           "value='%s'></label>", router);
  response.write(html);
  snprintf(html, sizeof(html),
           "<label>Port KNXnet/IP<input name='knx_port' type='number' min='1' "
           "max='65535' required value='%u'></label>", general.port);
  response.write(html);
  snprintf(html, sizeof(html),
           "<label>Multicast Routing<input name='multicast_ip' required value='%s'></label>",
           multicast);
  response.write(html);
  snprintf(html, sizeof(html),
           "<label>Timeout retour d'etat (ms)<input name='feedback_timeout_ms' "
           "type='number' min='1000' max='60000' required value='%lu'></label></fieldset>",
           static_cast<unsigned long>(general.feedbackTimeoutMs));
  response.write(html);
  for (uint8_t index = 0; index < 4; ++index) {
    const auto& button = store_->data().buttons[index];
    char escapedName[128]{};
    char command[16]{};
    char status[16]{};
    char html[192]{};
    htmlEscape(button.name, escapedName, sizeof(escapedName));
    KnxTouchConfigStore::formatGroupAddress(button.commandAddress, command, sizeof(command));
    KnxTouchConfigStore::formatGroupAddress(button.statusAddress, status, sizeof(status));
    snprintf(html, sizeof(html), "<fieldset><legend>Touche %u</legend><label>Nom<input name='name%u' maxlength='23' required value='",
             index + 1, index);
    response.write(html);
    response.write(escapedName);
    snprintf(html, sizeof(html),
             "'></label><label>Groupe commande<input name='command%u' required "
             "pattern='[0-9]{1,2}/[0-7]/[0-9]{1,3}' value='%s'></label>",
             index, command);
    response.write(html);
    snprintf(html, sizeof(html),
             "<label>Groupe statut<input name='status%u' required "
             "pattern='[0-9]{1,2}/[0-7]/[0-9]{1,3}' value='%s'></label>",
             index, status);
    response.write(html);
    snprintf(html, sizeof(html),
             "<label><input type='checkbox' name='enabled%u' value='1'%s> "
             "Touche active</label></fieldset>",
             index, button.enabled ? " checked" : "");
    response.write(html);
  }
  response.write("<button type='submit'>Enregistrer</button></form></section>");
  response.endPage();
}
