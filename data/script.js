const ESP_IP = "http://10.0.0.26"; // Změňte na IP adresu vašeho ESP8266

function fetchData() {
    fetch(ESP_IP)
        .then(response => response.json()) // Zde bychom mohli pracovat s JSON
        .then(data => {
            document.getElementById('temperature').innerText = data.temperature + ' °C';
            document.getElementById('humidity').innerText = data.humidity + ' %';
            document.getElementById('waterLevel').innerText = data.waterLevel;
        })
        .catch(error => {
            console.error('Chyba:', error);
        });
}

// Aktualizace dat každých 5 sekund
setInterval(fetchData, 5000);
fetchData(); // První načtení dat