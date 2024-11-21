async function loadStatistics() {
    try {
        const response = await fetch('/statistics');
        if (!response.ok) {
            throw new Error('Data nejsou k dispozici.');
        }

        const data = await response.json();
        if (!data || data.length === 0) {
            throw new Error('Žádná data k dispozici.');
        }

        const labels = data.map((_, index) => `Den ${index + 1}`);
        const tempData = data.map(day => day.temp_avg);
        const humidityData = data.map(day => day.humidity_avg);

        // Naplnění tabulky statistik
        const tableBody = document.getElementById("dailyStats").getElementsByTagName("tbody")[0];
        tableBody.innerHTML = ""; // Vyprázdnit tabulku

        data.forEach((day, index) => {
            const row = document.createElement("tr");
            row.innerHTML = `
                <td>Den ${index + 1}</td>
                <td>${day.temp_avg.toFixed(1)} °C</td>
                <td>${day.humidity_avg.toFixed(1)} %</td>
            `;
            tableBody.appendChild(row);
        });

        // Vykreslení grafu
        const ctx = document.getElementById('chart').getContext('2d');
        new Chart(ctx, {
            type: 'line',
            data: {
                labels: labels,
                datasets: [
                    {
                        label: 'Průměrná teplota (°C)',
                        data: tempData,
                        borderColor: 'rgba(255, 99, 132, 1)',
                        borderWidth: 2,
                        fill: false
                    },
                    {
                        label: 'Průměrná vlhkost (%)',
                        data: humidityData,
                        borderColor: 'rgba(54, 162, 235, 1)',
                        borderWidth: 2,
                        fill: false
                    }
                ]
            },
            options: {
                responsive: true,
                scales: {
                    y: { beginAtZero: true }
                }
            }
        });
    } catch (error) {
        console.error('Chyba při načítání statistik:', error.message);
        displayErrorMessage(error.message);
    }
}

async function loadSensorData() {
    try {
        const response = await fetch('/sensorData');
        if (!response.ok) {
            throw new Error('Data senzorů nejsou dostupná.');
        }

        const data = await response.json();
        document.getElementById('temperature').innerText = `${data.temperature} °C`;
        document.getElementById('humidity').innerText = `${data.humidity} %`;
        document.getElementById('waterLevel').innerText = data.waterLevel;
        document.getElementById('soilMoisture').innerText = data.soilMoisture ? 'Vlhká' : 'Suchá';
        document.getElementById('pumpState').innerText = data.pumpState ? 'Zapnuto' : 'Vypnuto';

        const pumpButton = document.getElementById('togglePumpBtn');
        pumpButton.innerText = data.pumpState ? 'Vypnout pumpu' : 'Zapnout pumpu';
    } catch (error) {
        console.error('Chyba při načítání dat ze senzorů:', error);
        displayErrorMessage(error.message);
    }
}

async function togglePump() {
    try {
        const response = await fetch('/togglePump');
        if (!response.ok) {
            throw new Error('Chyba při přepínání pumpy.');
        }
        const message = await response.text();
        console.log(message);
        loadSensorData(); // Aktualizuj data po přepnutí pumpy
    } catch (error) {
        console.error('Chyba při přepínání pumpy:', error);
        displayErrorMessage(error.message);
    }
}

function displayErrorMessage(message) {
    alert(message);
    const tableBody = document.getElementById("dailyStats").getElementsByTagName("tbody")[0];
    tableBody.innerHTML = "<tr><td colspan='3'>Data nejsou k dispozici.</td></tr>";

    const ctx = document.getElementById('chart').getContext('2d');
    new Chart(ctx, {
        type: 'line',
        data: {
            labels: ['Den 1'],
            datasets: [
                {
                    label: 'Průměrná teplota (°C)',
                    data: [0],
                    borderColor: 'rgba(255, 99, 132, 1)',
                    borderWidth: 2,
                    fill: false
                },
                {
                    label: 'Průměrná vlhkost (%)',
                    data: [0],
                    borderColor: 'rgba(54, 162, 235, 1)',
                    borderWidth: 2,
                    fill: false
                }
            ]
        },
        options: {
            responsive: true,
            scales: {
                y: { beginAtZero: true }
            }
        }
    });
}

document.addEventListener('DOMContentLoaded', () => {
    loadSensorData();
    loadStatistics();
    setInterval(loadSensorData, 10000); // Obnovování dat každých 10 sekund
});