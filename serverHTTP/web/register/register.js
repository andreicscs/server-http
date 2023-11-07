document.addEventListener('DOMContentLoaded', function () {
    document.querySelector('form').addEventListener('submit', async function (event) {
        event.preventDefault(); // Prevent the form from submitting
        // Get the input values
        const username = document.getElementById('username').value;
        const email = document.getElementById('email').value;
        const password = document.getElementById('password').value;
    
        // Construct the request body as plain text
        const requestBody = 'username=' + encodeURIComponent(username) + '&' + 'email=' + encodeURIComponent(email) + '&' + 'password=' + encodeURIComponent(password);
    
        try {
            // Make a fetch request to your server-side authentication script
            const response = await fetch('/register', {
                method: 'POST',
                headers: {
                    'Content-Type': 'text/plain' // Set the content type to text/plain
                },
                body: requestBody,
                credentials: 'include' // Include cookies in the request
            });
    
            if (response.ok) {
                window.location.href = '/login';
            } else if(response.statusText.localeCompare("account already exists")==0){
                document.getElementsByClassName('UsernameInput')[0].classList.add('incorrect-input');
                document.getElementsByClassName('EmailInput')[0].classList.add('incorrect-input');
                document.getElementsByClassName('PasswordInput')[0].classList.add('incorrect-input');
                // Listen for the transitionend event on a relevant element (e.g., username input)
                document.getElementsByClassName('UsernameInput')[0].addEventListener('animationend', function() {
                // Remove the class when the transition ends
                document.getElementsByClassName('EmailInput')[0].classList.remove('incorrect-input');
                document.getElementsByClassName('UsernameInput')[0].classList.remove('incorrect-input');
                document.getElementsByClassName('PasswordInput')[0].classList.remove('incorrect-input');
                });
            }else if(response.statusText.localeCompare("invalid username")==0){
                document.getElementsByClassName('UsernameInput')[0].classList.add('incorrect-input');
                // Listen for the transitionend event on a relevant element (e.g., username input)
                document.getElementsByClassName('UsernameInput')[0].addEventListener('animationend', function() {
                // Remove the class when the transition ends
                document.getElementsByClassName('UsernameInput')[0].classList.remove('incorrect-input');
                });
            }else if(response.statusText.localeCompare("invalid email")==0){
                document.getElementsByClassName('EmailInput')[0].classList.add('incorrect-input');
                document.getElementsByClassName('EmailInput')[0].addEventListener('animationend', function() {
                    // Remove the class when the transition ends
                    document.getElementsByClassName('EmailInput')[0].classList.remove('incorrect-input');
                });
            }else if(response.statusText.localeCompare("invalid password")==0){
                document.getElementsByClassName('PasswordInput')[0].classList.add('incorrect-input');
                // Listen for the transitionend event on a relevant element (e.g., username input)
                document.getElementsByClassName('PasswordInput')[0].addEventListener('animationend', function() {
                    // Remove the class when the transition ends
                    document.getElementsByClassName('PasswordInput')[0].classList.remove('incorrect-input');
                });
            }
            
        } catch (error) {
            // Handle network or fetch errors here
            console.error('Fetch Error:', error);
        }
    });
});