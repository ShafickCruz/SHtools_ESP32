const fileInput = document.getElementById("css_fileInput");
const fileInfo = document.getElementById("css_fileInfo");
const fileTexto = document.getElementById("css_fileInput-texto");
const fileButton = document.getElementById("css_filebutton");
const fileInput_max_size = 1310720; // 1.25MB em bytes
const dirInput = document.getElementById("css_dirInput");
const dirInfo = document.getElementById("css_dirInfo");
const dirTexto = document.getElementById("css_dirInput-texto");
const dirButton = document.getElementById("css_dirbutton");
const uploadStatus = document.getElementById("css_uploadStatus");
const pBar = document.getElementById("css_progressBar");
const uploadButton = document.getElementById("css_uploadButton");
const checksum = document.getElementById("id_checksum");
const testarFirmware = document.getElementById("id_testarFirmware");
const inputsTitulo = document.getElementById("css_inputs-titulo");
let placaInfo = {};
const placa = document.getElementById("id_placa");
const placaIcon = document.getElementById("id_placa-icone");
const ip = window.location.origin; // Obtém o esquema e o IP do ambiente do navegador (obtem o que estiver sendo usado: http://ip, https://ip, domino, etc.)
const HeaderOTA = "X-OTA-Header";
const HeaderOTAassinatura = "SHtoolsOTA";
const HeaderOTAtestarFirmware = "X-TestarFirmware";
const ParamChecksum = "checksum";

/*********************/
/***** CABEÇALHO *****/
/*********************/
function js_set_tema(tema) {
  document.body.className = tema + "-theme";
  document
    .querySelectorAll(".css_toggle-tema button")
    .forEach((btn) => btn.classList.remove("active"));
  document
    .querySelector(`.css_toggle-tema button[onclick="js_set_tema('${tema}')"]`)
    .classList.add("active");
}

/*********************/
/***** FILEINPUT *****/
/*********************/
function handle_inputFileChange() {
  const file = fileInput.files[0];
  if (file) {
    uploadStatus.innerHTML = ""; // limpa div de informação de upload

    if (!file.name.endsWith(".bin")) {
      const continueUpload = confirm(
        "O formato do arquivo não é o esperado (.bin), deseja continuar mesmo assim?"
      );
      if (!continueUpload) {
        js_limparInput();
        return;
      }
    }
    fileTexto.value = file.name;
    fileInfo.innerHTML = `<strong>Nome:</strong> ${
      file.name
    } | <strong>Tipo:</strong> ${
      file.type || "unknown"
    } | <strong>Tamanho:</strong> ${(file.size / 1024).toFixed(2)} KB`;

    js_validarFileInput(file);
  } else {
    js_limparInput();
  }
}

function js_limparInput() {
  if (fileButton.disabled === false) {
    fileInput.value = "";
    fileTexto.value = "";
    fileInfo.innerHTML = "";
  } else if (dirButton.disabled === false) {
    dirInput.value = "";
    dirTexto.value = "";
    dirInfo.innerHTML = "";
  }
  js_ControlesHandle(true);
}

function js_validarFileInput(_firmware) {
  if (_firmware) {
    /* SIZE */
    if (_firmware.size > fileInput_max_size) {
      alert(
        `O arquivo selecionado excede o tamanho máximo permitido de ${(
          fileInput_max_size / 1024
        ).toFixed(2)} KB!`
      );
      js_limparInput();
      js_ControlesHandle(true);
      return;
    }

    /* CABEÇALHO E LEITURA*/
    const fileReader = new FileReader();

    // Evento para lidar com erro de leitura
    fileReader.onerror = function () {
      alert("Erro ao ler o arquivo. Por favor, tente novamente.");
      js_limparInput();
      js_ControlesHandle(true);
      return;
    };

    // Ler os primeiros 1 byte do arquivo
    fileReader.readAsArrayBuffer(_firmware.slice(0, 1));

    // Evento para lidar com a leitura bem-sucedida
    fileReader.onload = function (event) {
      const arrayBuffer = event.target.result;
      const bytes = new Uint8Array(arrayBuffer);

      // Verificando o primeiro byte
      const expectedHeader = 0xe9; // Valor esperado no primeiro byte
      if (bytes[0] !== expectedHeader) {
        const continueUpload = confirm(
          "O arquivo não possui o formato magic number/cabeçalho esperado. Deseja continuar assim mesmo?"
        );
        if (!continueUpload) {
          js_limparInput();
          js_ControlesHandle(true);
          return; // Sai da função se o usuário não quiser continuar
        }
      }
    };

    js_ControlesHandle(false);
  } else {
    js_limparInput();
    js_ControlesHandle(true);
  }
}

/*********************************/
/***** TOGGLE TIPO DE UPLOAD *****/
/*********************************/
function set_tipoUpload(tipo) {
  /* temporario enquanto nao é implementado LittleFS */
  if (tipo === "littlefs") {
    alert("LittleFS não está indisponível (em desenvolvimento).");
    js_limparInput();
    set_tipoUpload("firmware");
    return;
  }
  /**************************************************/

  if (tipo === "firmware") {
    inputsTitulo.innerHTML = "Selecione o firmware:";
  } else if (tipo === "littlefs") {
    inputsTitulo.innerHTML = "Selecione o diretório:";
  }

  fileButton.disabled = tipo !== "firmware";
  fileTexto.disabled = tipo !== "firmware";
  dirButton.disabled = tipo !== "littlefs";
  dirTexto.disabled = tipo !== "littlefs";

  document
    .getElementById("firmwareOption")
    .classList.toggle("active", tipo === "firmware");
  document
    .getElementById("littlefsOption")
    .classList.toggle("active", tipo === "littlefs");
}

/******************/
/***** PLACA  *****/
/******************/
function js_placa() {
  let _ok = false;

  // Limpa e desabilita controles
  js_ControlesHandle(true);
  js_limparInput();

  // Exibe o loader
  js_loader(true);

  // Solicita informações da placa ao servidor
  const url = "/placa_info";
  const options = {
    method: "GET",
    headers: {
      [HeaderOTA]: HeaderOTAassinatura,
    },
  };

  js_placaRequest(url, options)
    .then((data) => {
      placaInfo = data; // Preenche placaInfo com os dados recebidos
      // Verificação se houve erro
      if (placaInfo["error"]) {
        alert(
          "Falha ao tentar obter informações da Placa ESP32:\n" +
            placaInfo["error"]
        );
      } else {
        // variavel constante para preenchimento do tooltip
        const placa_tooltip_info = [
          `<div><strong>Chip:</strong> ${
            placaInfo.chip
          } (revisão ${placaInfo.revisao
            .split(".")
            .map(Number)
            .concat(0, 0)
            .slice(0, 3)
            .join(".")})</div>
           <div><strong>CPU:</strong> ${placaInfo.cpuFreqMhz}, ${
            placaInfo.nucleos
          } núcleos</div>
           <div><strong>Cristal:</strong> ${placaInfo.cristal}</div>
           <div><strong>MAC:</strong> ${placaInfo.mac}</div>
           <div><strong>Recursos:</strong> 
            <span class="${placaInfo.wifi ? "" : "indisponivel"}">WiFi</span>, 
            <span class="${
              placaInfo.bt ? "" : "indisponivel"
            }">Bluetooth</span>, 
            <span class="${placaInfo.ble ? "" : "indisponivel"}">BLE</span>
          </div>
           <div><strong>Flash:</strong> ${placaInfo.flashType}, ${
            placaInfo.flashSpeed
          }, ${placaInfo.flashSize} total, ${
            placaInfo.flashDisponivel
          } livre</div>
           <div><strong>RAM:</strong> ${placaInfo.ramTotal} total, ${
            placaInfo.ramDisponivel
          } livre</div>`,
          "Placa não suportada",
          "Placa não detectada",
        ];

        // Verificação se é ESP32 e atualização da interface
        placa.classList.remove(
          "css_placa-correta",
          "css_placa-incorreta",
          "css_placa-desabilitado"
        );
        placaIcon.style.display = "none";
        let tooltipPlaca = ""; // Variável para armazenar o conteúdo do tooltip

        if (placaInfo["chipIsEP32"] === "true") {
          // ESP32 detectado
          placa.classList.add("css_placa-correta");
          placaIcon.innerHTML = "✔";
          placaIcon.style.display = "block";
          // Atualizando o tooltip com informações detalhadas
          tooltipPlaca = placa_tooltip_info[0];
          _ok = true;
        } else if (placaInfo["chipIsEP32"] === "false") {
          // Não é ESP32
          placa.classList.add("css_placa-incorreta");
          placaIcon.innerHTML = "✖";
          placaIcon.style.display = "block";

          // Tooltip indicando placa incorreta
          tooltipPlaca = placa_tooltip_info[1];
        } else {
          // Placa não detectada
          placa.classList.add("css_placa-desabilitado");

          // Tooltip indicando que a placa não foi detectada
          tooltipPlaca = placa_tooltip_info[2];
        }

        // Atualizando o conteúdo do tooltip
        document.getElementById("css_placa-tooltip").innerHTML = tooltipPlaca;
      }

      if (!_ok) {
        js_limparInput();
        js_ControlesHandle(true);
      }
    })
    .catch((error) => {
      placaInfo.error = error.message; // Adiciona a propriedade error em caso de erro
      alert("Erro ao obter informações da Placa ESP32:\n" + error.message);
      placa.classList.add("css_placa-desabilitado");
    })
    .finally(() => {
      js_loader(false); // Esconde o loader ao final, independentemente do resultado
    });
}

function js_placaRequest(url, options = {}, timeout = 5000) {
  return new Promise((resolve, reject) => {
    const controller = new AbortController();
    const signal = controller.signal;
    // Configurar timeout
    const timeoutId = setTimeout(() => {
      controller.abort(); // Aborta a requisição
      reject(new Error("Request timed out"));
    }, timeout);

    fetch(url, { ...options, signal })
      .then((response) => {
        clearTimeout(timeoutId); // Limpa o timeout
        if (!response.ok) {
          // Cria um erro com o código de status
          return response.text().then((message) => {
            reject(new Error(`Error ${response.status}: ${message}`)); // Captura a mensagem personalizada
          });
          /*
          reject(
            new Error(
              `Network response was not ok: ${response.status} ${response.statusText}`
            )
          );*/
        }
        return response.json(); // Retorna a resposta em formato JSON
      })
      .then((data) => {
        resolve(data);
      }) // Resolve a Promise com os dados
      .catch((error) => {
        // Captura erros e fornece informações adicionais
        if (error.name === "AbortError") {
          reject(new Error("Request was aborted"));
        } else {
          reject(error);
        }
      });
  });
}

/********************/
/***** CHECKSUM *****/
/********************/
/**
 * Função para calcular o checksum do arquivo (usando SHA-256)
 */
function js_checksum(file) {
  return new Promise((resolve, reject) => {
    const reader = new FileReader();
    reader.onerror = () => {
      reject(new Error("Erro ao ler o arquivo para cálculo de checksum."));
    };

    reader.onload = (event) => {
      const arrayBuffer = event.target.result;
      const shaObj = new jsSHA("SHA-256", "ARRAYBUFFER");
      shaObj.update(arrayBuffer);
      const hashHex = shaObj.getHash("HEX");
      resolve(hashHex);
    };

    reader.readAsArrayBuffer(file);
  });
}

/************************/
/***** BOTÃO UPLOAD *****/
/************************/

async function js_uploadButton() {
  if (uploadButton.disabled) {
    alert("O botão está desabilitado. Não é possível fazer o upload.");
    return;
  }

  const file = fileInput.files[0];
  if (!file) {
    alert("Nenhum arquivo selecionado.");
    return;
  }

  js_loader(true);

  const testeDEfirmware = testarFirmware.checked;
  let uploadOK = false;

  // Iniciando ações
  try {
    const formData = new FormData();
    formData.append("file", file);

    // Verifica se o checkbox de checksum está marcado e se estiver, inicia contagem
    if (checksum.checked) {
      try {
        const _checksum = await js_checksum(file);
        formData.append(ParamChecksum, _checksum);
      } catch (error) {
        alert("Erro ao calcular checksum.\nDetalhes: " + error.message);
        return;
      }
    }

    // Inicia o upload
    const response = await js_firmwareTOserver(formData, testeDEfirmware); // Passa o estado do checkbox de teste de firmware

    // Trata as respostas recebidas: sucesso ou falha
    if (response.resultado === true) {
      uploadOK = true;
      uploadStatus.innerHTML = `
        <span class='css_uploadStatus-sucesso'><strong>SUCESSO:</strong> ${response.msg}</span>`;
    } else {
      uploadStatus.innerHTML = `
      <span class='css_uploadStatus-falha'><strong>ERRO:</strong> ${response.msg}</span>`;
    }
  } catch (error) {
    uploadStatus.innerHTML = `
    <span class='css_uploadStatus-falha'><strong>ERRO:</strong> ${error.message}</span>`;
    return;
  } finally {
    if (uploadOK) {
      // UPLOAD BEM SUCEDIDO
      js_uploadButton_final(testeDEfirmware);
    } else {
      alert("FALHA!");
      js_limparInput();
      uploadStatus.innerHTML = "";
      js_ControlesHandle(true);
      js_loader(false);
    }
  }
}

function js_firmwareTOserver(formData, _testeDEfirmware) {
  return new Promise((resolve, reject) => {
    const xhr = new XMLHttpRequest();
    js_firmwareTOserver_prepararRequest(xhr, _testeDEfirmware); // abre e formata o cabeçalho do request
    js_firmwareTOserver_progresso(xhr); // trata a barra de progressso
    js_firmwareTOserver_resposta(xhr, resolve, reject); // trata as respostas recebidas do servidor
    xhr.send(formData); // Envia a requisição para o servidor
  });
}

function js_firmwareTOserver_prepararRequest(xhr, _testeDEfirmware) {
  xhr.open("POST", "/upload_firmware", true);
  xhr.setRequestHeader(HeaderOTA, HeaderOTAassinatura);
  if (_testeDEfirmware) {
    xhr.setRequestHeader(HeaderOTAtestarFirmware, "true");
  }
}

function js_firmwareTOserver_progresso(xhr) {
  xhr.upload.onprogress = function (event) {
    if (event.lengthComputable) {
      const percentComplete = (event.loaded / event.total) * 100;
      pBar.value = percentComplete; // Atualiza a barra de progresso
    }
  };
}

function js_firmwareTOserver_resposta(xhr, resolve, reject) {
  xhr.onload = function () {
    if (xhr.status === 200) {
      resolve({ resultado: true, msg: xhr.responseText });
    } else {
      resolve({
        resultado: false,
        msg: `${xhr.responseText}<br><strong>Detalhes:</strong> ${xhr.status}: ${xhr.statusText}`,
      });
    }
  };
  xhr.onerror = function () {
    reject(
      new Error(
        "Erro de conexão.<br><strong>Detalhes:</strong> Erro na conexão com o servidor."
      )
    );
  };
}

async function js_uploadButton_final(_testeDEfirmware) {
  if (!_testeDEfirmware) {
    // sem teste de firmware
    showToast(
      "O novo firmware será ativado <strong>SEM TESTE</strong> de firmware ou efetuará rollback em caso de falha.",
      "UPLOAD CONLUÍDO",
      5000
    );
  } else {
    // com teste de firmware
    uploadStatus.innerHTML +=
      "<br>O servidor irá iniciar o processo de teste do novo firmware. Aguarde...";
    // Inicia o monitoramento da conexão e aguarda o resultado

    //spinner.style.borderRightColor = "#00FF00";
    const resultado = await monitorarConexao(uploadStatus);

    js_loader(false);

    switch (resultado) {
      case 0:
        showToast(
          "Houve falha na tentativa de conexão com o servidor.",
          `<span class="css_uploadStatus-falha">
        <strong>"FALHA"</strong>
      </span>`,
          5000
        );
        break;

      case 1:
        showToast(
          "O novo firmware foi testado e aprovado!",
          `<span class="css_uploadStatus-sucesso"><strong>SUCESSO!</strong></span>`,
          5000
        );
        break;

      case 2:
        showToast(
          "Houve erro no processo de teste do novo firmware.",
          `<span class="css_uploadStatus-falha">
          <strong>"ERRO"</strong>
        </span>`,
          5000
        );
        break;

      default:
        showToast(
          "Erro desconhecido.",
          `<span class="css_uploadStatus-falha">
        <strong>"FALHA"</strong>
      </span>`,
          5000
        );
        break;
    }
  }

  js_limparInput();
  js_ControlesHandle(true);
}

// Função para monitorar a conexão e atualizar o status
async function monitorarConexao(uploadStatus) {
  const timeout = 10000; // Timeout total de 10 segundos
  const endTime = Date.now() + timeout;

  return new Promise((resolve) => {
    const interval = setInterval(() => {
      const xhr = new XMLHttpRequest();
      xhr.open("GET", `${ip}/server_status`, true); // Endpoint para verificar status
      xhr.setRequestHeader(HeaderOTA, HeaderOTAassinatura);
      xhr.timeout = 2000; // Timeout para a requisição

      xhr.onload = function () {
        uploadStatus.innerHTML += "<br>";

        switch (xhr.status) {
          case 200:
            uploadStatus.innerHTML += `<span class='css_uploadStatus-sucesso'><strong>SUCESSO:</strong> ${xhr.responseText}</span>`;
            clearInterval(interval);
            resolve(1); // Retorna 1 para sucesso
            break;

          case 403:
          case 500:
            uploadStatus.innerHTML += `<span class='css_uploadStatus-falha'><strong>ERRO:</strong> ${xhr.responseText}<br><strong>${xhr.status}</strong>: ${xhr.statusText}</span>`;
            clearInterval(interval);
            resolve(2); // Retorna 2 para erro
            break;

          case 202:
            //spinner.style.borderRightColor = "#00FF00"; // Verde fluorescente
            js_loader_cor("verde");
            break;

          default:
            //spinner.style.borderRightColor = "#FF0000"; // Vermelho fluorescente
            js_loader_cor("vermelho");
            break;
        }
      };

      xhr.onerror = function () {
        //spinner.style.borderRightColor = "#FF0000"; // Vermelho fluorescente
        js_loader_cor("vermelho");
      };

      xhr.ontimeout = function () {
        //spinner.style.borderRightColor = "#FF0000"; // Vermelho fluorescente
        js_loader_cor("vermelho");
      };

      xhr.send();

      if (Date.now() > endTime) {
        uploadStatus.innerHTML += `<span class='css_uploadStatus-falha'><strong>TIMEOUT:</strong> Não foi possível reestabelecer conexão automática com o servidor.</span>`;
        clearInterval(interval);
        resolve(0); // Retorna 0 para timeout
      }
    }, 500); // Verifica a cada 500ms
  });
}

/*****************/
/***** GERAL *****/
/*****************/
window.onload = function () {
  set_tipoUpload("firmware");
  js_set_tema("light");
  js_placa();
};

function js_ControlesHandle(_disable) {
  uploadButton.disabled = _disable;
  checksum.checked = !_disable;
  checksum.disabled = _disable;
  testarFirmware.checked = !_disable;
  testarFirmware.disabled = _disable;
  if (_disable) {
    pBar.value = 0;
  }
}

function js_loader(_show) {
  if (_show) {
    document.getElementById("css_loader-overlay").style.display = "flex"; // Mostra o loader
  } else {
    document.getElementById("css_loader-overlay").style.display = "none"; // Esconde o loader
  }
}

function js_loader_cor(cor) {
  const loader = document.getElementById("css_loader");
  loader.classList.remove("css_loader-vermelho", "css_loader-verde");
  if (cor === "verde") {
    loader.classList.add("css_loader-verde");
  } else if (cor === "vermelho") {
    loader.classList.add("css_loader-vermelho");
  }
}

async function await_Delay(ms) {
  await new Promise((resolve) => setTimeout(resolve, ms));
}

let toastTimeout;

function showToast(message, title = "", duration = 5000) {
  const toast = document.getElementById("id_toast");
  toast.innerHTML = title ? `<strong>${title}</strong><br>${message}` : message;
  toast.className = "show";

  // Limpa o timeout anterior se existir
  if (toastTimeout) {
    clearTimeout(toastTimeout);
  }

  // Define um novo timeout
  toastTimeout = setTimeout(() => {
    toast.className = toast.className.replace("show", "");
  }, duration);
}
