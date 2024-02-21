document.addEventListener('DOMContentLoaded', function() {
    // Function to fetch and display data
    function fetchData() {
        var xhr = new XMLHttpRequest();
        xhr.open('GET', '/get_data/' + document.body.getAttribute('data-username'), true);
        xhr.onreadystatechange = function() {
            if (xhr.readyState === XMLHttpRequest.DONE) {
                if (xhr.status === 200) {
                    var response = JSON.parse(xhr.responseText);
                    displayData(response.open_drinks);
                    document.getElementById("username").textContent = response.username;
                    document.getElementById("current_puk_id").textContent = response.puk_key ? response.puk_key : "No Puk-ID configured!";
                } else {
                    console.error('Error:', xhr.status);
                }
            }
        };
        xhr.send();
    }

    // Function to display the received data
    function displayData(data) {
        var dataTable = document.getElementById('drinks_table');
        var tbody = dataTable.querySelector('tbody');
        tbody.innerHTML = ''; // Clear previous data

        data.forEach(function(item) {
            var row = tbody.insertRow();
            var nameCell = row.insertCell();
            var priceCell = row.insertCell();
            nameCell.textContent = item.name;
            priceCell.textContent = item.price + " €";
        });
    }

    // Function to periodically update the data
    function updateData() {
        fetchData();
        setInterval(fetchData, 5000); // Update data every 5 seconds
    }

    updateData(); // Start updating data

    // Form event listeners
    document.getElementById('esp32-form').addEventListener('submit', function(event) {
        event.preventDefault(); // Prevent default form submission behavior
        var esp32_id = document.getElementById('esp32-id').value;
        setPukId(esp32_id); // Send data to Python function
    });

    document.getElementById('esp32-form').addEventListener('reset', function(event) {
        event.preventDefault(); // Prevent default form submission behavior
        setPukId(""); // Reset Puk-ID
    });

    document.getElementById('pay-form').addEventListener('submit', function(event) {
        event.preventDefault(); // Prevent default form submission behavior
        pay(); // Send data to Python function
    });

    // Function to set Puk-ID
    async function setPukId(espId) {
        const url = '/puk_id_data';
        if (!espId) espId = null;
        const data = { esp_id: espId, username: document.body.getAttribute('data-username') };
        fetch(url, { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(data) });
    }

    // Function to handle payment
    async function pay() {
        const url = '/pay_bill';
        const data = { username: document.body.getAttribute('data-username') };

        try {
            const response = await fetch(url, { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(data) });

            if (!response.ok) {
                throw new Error('Failed to fetch data');
            }

            const responseText = await response.text();

            if (responseText.trim() === 'None') {
                alert('Cannot create invoice because the drink list is empty.');
            } else {
                html2pdf().from(responseText).toPdf().get('pdf').then(function(pdf) {
                    const pdfDataUri = pdf.output('datauristring');
                    const byteCharacters = atob(pdfDataUri.split(',')[1]);
                    const byteNumbers = new Array(byteCharacters.length);
                    for (let i = 0; i < byteCharacters.length; i++) {
                        byteNumbers[i] = byteCharacters.charCodeAt(i);
                    }
                    const byteArray = new Uint8Array(byteNumbers);
                    const blob = new Blob([byteArray], { type: 'application/pdf' });
                    const blobUrl = URL.createObjectURL(blob);
                    const popup = window.open(blobUrl, '_blank');
                    URL.revokeObjectURL(blobUrl);
                });
            }
        } catch (error) {
            console.error('Error:', error);
            alert('Rechnung kann nicht erstellt werden, da keine gebuchten Getränke vorhanden.');
        }
    }
});
