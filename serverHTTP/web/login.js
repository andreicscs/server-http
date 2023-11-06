document.addEventListener('DOMContentLoaded', function () {
    document.querySelector('form').addEventListener('submit', async function (event) {
        event.preventDefault(); // Prevent the form from submitting
        // Get the input values
        const email = document.getElementById('email').value;
        const password = document.getElementById('password').value;
    
        // Construct the request body as plain text
        const requestBody = 'email=' + encodeURIComponent(email) + '&' + 'password=' + encodeURIComponent(password);
    
        try {
            // Make a fetch request to your server-side authentication script
            const response = await fetch('/login', {
                method: 'POST',
                headers: {
                    'Content-Type': 'text/plain' // Set the content type to text/plain
                },
                body: requestBody,
                credentials: 'include' // Include cookies in the request
            });
    
            if (response.ok) {
                window.location.href = '/userData.html';
            } else {
                // Handle HTTP errors here, e.g., display an error message
                //console.error('HTTP Error:', response.status, response.statusText);

                if(response.statusText.localeCompare("email not registered")==0 || response.statusText.localeCompare("invalid email")==0){
                    document.getElementsByClassName('EmailInput')[0].classList.add('incorrect-input');
                    document.getElementsByClassName('PasswordInput')[0].classList.add('incorrect-input');
                    document.getElementsByClassName('EmailInput')[0].addEventListener('animationend', function() {
                        // Remove the class when the transition ends
                        document.getElementsByClassName('EmailInput')[0].classList.remove('incorrect-input');
                        document.getElementsByClassName('PasswordInput')[0].classList.remove('incorrect-input');
                    });
                }else if(response.statusText.localeCompare("wrong password")==0 || response.statusText.localeCompare("invalid password")==0){
                                                          document.getElementsByClassName('PasswordInput')[0].classList.add('incorrect-input');
                    // Listen for the transitionend event on a relevant element (e.g., username input)
                    document.getElementsByClassName('PasswordInput')[0].addEventListener('animationend', function() {
                        // Remove the class when the transition ends
                        document.getElementsByClassName('PasswordInput')[0].classList.remove('incorrect-input');
                    });
                }
            }
        } catch (error) {
            // Handle network or fetch errors here
            console.error('Fetch Error:', error);
        }
    });
});