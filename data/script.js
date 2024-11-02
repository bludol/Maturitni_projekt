const ESP_IP = "http://10.0.0.11"; // Změňte na IP adresu vašeho ESP8266

function fetchData() {
    fetch(ESP_IP)
        .then(response => response.text()) // Zde očekáváme HTML
        .then(data => {
            const parser = new DOMParser();
            const doc = parser.parseFromString(data, 'text/html');

            document.getElementById('temperature').innerText = doc.getElementById('temperature').innerText;
            document.getElementById('humidity').innerText = doc.getElementById('humidity').innerText;
            document.getElementById('waterLevel').innerText = doc.getElementById('waterLevel').innerText;
            document.getElementById('soilMoisture').innerText = doc.getElementById('soilMoisture').innerText;
            document.getElementById('pumpState').innerText = doc.getElementById('pumpState').innerText;
            document.getElementById('togglePumpBtn').innerText = doc.getElementById('pumpState').innerText === 'Zapnuto' ? 'Vypnout pumpu' : 'Zapnout pumpu';
        })
        .catch(error => {
            console.error('Chyba:', error);
        });
}

// Přidání event listeneru pro tlačítko
document.getElementById('togglePumpBtn').addEventListener('click', function() {
    fetch(`${ESP_IP}/togglePump`)
        .then(response => response.text())
        .then(message => {
            console.log(message);
            fetchData(); // Znovu načíst data po změně stavu pumpy
        })
        .catch(error => {
            console.error('Chyba při přepínání pumpy:', error);
        });
});

// Aktualizace dat každých 5 sekund
setInterval(fetchData, 5000);
fetchData(); // První načtení dat