
// Define an async function to use await
async function fetchData() {
    try {
        // Make a fetch request to your server-side authentication script
        const response = await fetch('/userData', {
            method: 'POST',
            headers: {
                'Content-Type': 'text/plain' // Set the content type to text/plain
            },
            credentials: 'include' // Include cookies in the request
        });

        if (response.ok) {
            // Assuming the response is OK, meaning the request was successful
            const htmlContent = await response.text(); // Extract the HTML content from the response
            document.body.innerHTML = htmlContent;
            /*
            document.open(); // Open the document
            document.write(htmlContent); // Write the received HTML content to the document
            document.close(); // Close the document
            */
        } else {
            // Handle the case where the response is not OK (e.g., server error)
            console.error('Server returned an error:', response.status, response.statusText);
        }
    } catch (error) {
        // Handle network or fetch errors here
        console.error('Fetch Error:', error);
    }
}

// Call the async function to start fetching data
fetchData();
