const ESP_IP = "http://192.168.158.39"; // Změňte na IP adresu vašeho ESP8266

function fetchData() {
    fetch(ESP_IP)
        .then(response => {
            if (!response.ok) {
                throw new Error('Síťová odpověď nebyla ok');
            }
            return response.text(); // Načtení HTML jako text
        })
        .then(data => {
            document.open();
            document.write(data); // Přepsání celé stránky novými daty
            document.close();
        })
        .catch(error => {
            console.error('Chyba:', error);
        });
}

// Aktualizace dat každých 5 sekund
setInterval(fetchData, 5000);
fetchData(); // První načtení dat