// Inicialização das variáveis de controle do log
let logCount = 1; // Contador de logs
const logEntries = []; // Armazena as entradas de log
const logHeaderText = "Início da captura dos Logs"; // Texto do cabeçalho

const ip = window.location.hostname; // Obtém o IP automaticamente do ambiente do navegador
let ws;

function conectarWebSocket() {
  ws = new WebSocket(`ws://${ip}/ws_rota`);

  ws.onopen = function () {
    addLogMessage("WebSocket conectado.");
  };

  ws.onerror = function (error) {
    addLogMessage("Erro no WebSocket: " + error.message, "red");
  };

  ws.onclose = function () {
    addLogMessage("Conexão WebSocket fechada.", "orange");

    // Tenta reconectar após 5 segundos
    setTimeout(conectarWebSocket, 1000);
  };
}

// Função inicial para conectar ao WebSocket
conectarWebSocket();

// Evento quando uma mensagem é recebida
ws.onmessage = function (event) {
  const message = event.data; // Captura a mensagem recebida

  // Verifica se a mensagem é uma resposta a um comando
  if (message.startsWith("cmd:") && message.includes(":resultado=")) {
    const partes = message.split(":resultado=");
    const resultado = partes[1].trim();

    const logMessage = `${partes[0]} ${
      resultado === "1" ? "executado com sucesso" : "falhou"
    }`;
    const logColor = resultado === "1" ? "mediumseagreen" : "lightcoral"; // Verde para sucesso, vermelho para falha

    // Adiciona a mensagem ao log com a cor apropriada
    addLogMessage(logMessage, logColor);
  } else {
    // Mensagem normal
    addLogMessage(message); // Apenas adiciona em preto
  }
};

/**
 * Adiciona uma mensagem ao log
 * black = mensagem comum
 * red = erro
 * orange = alerta
 * dodgerblue = comando
 * mediumseagreen = comando sucesso
 * lightcoral = comando falha
 */
function addLogMessage(message, color = "black") {
  const log = document.getElementById("log");
  const logEntry = document.createElement("div");
  const currentTime = new Date().toLocaleTimeString(); // Obtém o tempo atual
  const logText = `${logCount} (${currentTime}): ${message}`; // Monta o texto do log

  // Atualiza o innerHTML para incluir a cor especificada
  logEntry.innerHTML = `<span class="line-number">${logCount} (${currentTime}):</span> <span style="color: ${color};">${message}</span>`;

  log.insertBefore(logEntry, log.firstChild); // Adiciona a nova entrada no início do log
  logEntries.push(logText); // Adiciona a entrada ao array de logs

  // Rola o log para mostrar a última entrada
  if (document.getElementById("autoScrollToggle").checked) {
    log.scrollTop = log.scrollHeight;
  }

  logCount++; // Incrementa o contador de logs
}

// Função para enviar um comando ao ESP32
function sendCommand() {
  const commandInput = document.getElementById("command");
  const message = commandInput.value; // Obtém a mensagem do input
  if (message) {
    //console.log("Comando enviado: " + message); // Verifica se o comando está sendo capturado
    addLogMessage(message, "dodgerblue"); // Adiciona a mensagem ao log
    sendMessageToESP32(message); // Envia a mensagem para o ESP32
    commandInput.value = ""; // Limpa o campo de entrada
    commandInput.value = "cmd:"; // Define o valor do campo de entrada
  } else {
    //console.log("Campo de comando vazio."); // Loga um aviso se o campo estiver vazio
  }
}

// Função para enviar a mensagem ao ESP32 através do websocket
function sendMessageToESP32(message) {
  if (ws.readyState === WebSocket.OPEN) {
    // Verifica se o WebSocket está aberto
    //console.log("Enviando mensagem: " + message); // Verifica o estado do WebSocket
    ws.send(message); // Envia a mensagem
  } else {
    addLogMessage("WebSocket não está conectado.", "red"); // informa o erro se não estiver conectado
    //console.log("Erro: WebSocket não está conectado."); // Loga um erro
  }
}

// Atualizar o cabeçalho com o IP local e hora atual
function updateLogHeader() {
  const currentTime = new Date().toLocaleTimeString(); // Obtém a hora atual no formato hh:mm:ss
  const logHeader = document.getElementById("logHeader");
  logHeader.innerHTML = `${logHeaderText} | IP: <span id="localIP">${ip}</span> | hora: <span id="time">${currentTime}</span>`; // Atualiza o cabeçalho
}

// Alterna o tema entre claro e escuro
function setTheme(theme) {
  document.body.className = theme + "-theme"; // Define a classe do body para o tema
  document
    .querySelectorAll(".segmented-control button")
    .forEach((btn) => btn.classList.remove("active"));
  document
    .querySelector(`.segmented-control button[onclick="setTheme('${theme}')"]`)
    .classList.add("active");
}

// Limpa o conteúdo do log e atualiza o cabeçalho
function clearLogs() {
  const log = document.getElementById("log");
  log.innerHTML = ""; // Limpa todo o conteúdo visual do log, incluindo o cabeçalho

  // Recria o elemento logHeader
  const logHeader = document.createElement("div");
  logHeader.className = "log-header";
  logHeader.id = "logHeader";
  log.appendChild(logHeader); // Adiciona o cabeçalho recriado ao log

  logEntries.length = 0; // Limpa as entradas de log
  logCount = 1; // Reseta o contador de logs
  updateLogHeader(); // Atualiza o novo cabeçalho com IP e millis
}

// Exporta os logs para um arquivo de texto
function exportLogs() {
  const header = document.querySelector(".log-header").innerText; // Captura o cabeçalho do log
  const logText = [header, ...logEntries].join("\n"); // Monta o texto do log
  const blob = new Blob([logText], { type: "text/plain" }); // Cria um blob do texto do log
  const url = URL.createObjectURL(blob); // Cria uma URL para o blob
  const a = document.createElement("a");
  a.href = url; // Define a URL para o link
  a.download = "logs.txt"; // Define o nome do arquivo a ser baixado
  a.click(); // Clica no link para iniciar o download
  URL.revokeObjectURL(url); // Revoga a URL após o download
}

// Adiciona um evento keypress para enviar comando ao pressionar ENTER
document
  .getElementById("command")
  .addEventListener("keypress", function (event) {
    if (event.key === "Enter") {
      sendCommand(); // Chama a função para enviar o comando
      event.preventDefault(); // Evita que o formulário seja enviado, se houver um
    }
  });

// Evento para alternar o estado do auto scroll
document
  .getElementById("autoScrollToggle")
  .addEventListener("change", (event) => {
    if (event.target.checked) {
      event.target.classList.add("active"); // Marca como ativo
    } else {
      event.target.classList.remove("active"); // Remove marcação de ativo
    }
  });

// Chamada para atualizar o cabeçalho ao carregar a página
document.addEventListener("DOMContentLoaded", (event) => {
  updateLogHeader(); // Atualiza o cabeçalho na inicialização
  document.getElementById("command").value = "cmd:"; // Define o valor inicial do campo de entrada
});

// Simular mensagens recebidas (opcional)
// setInterval(() => {
//   addLogMessage(`Simulated message ${logCount}`);
// }, 3000);
