const BASEURL = 'https://snackapi.zeusyboy.com/api';

const id = localStorage.getItem('id') || crypto.randomUUID();
localStorage.setItem('id', id);
console.log(`User ID: ${id}`);

const yawElement = document.getElementById('yaw');
const pitchElement = document.getElementById('pitch');
const fireElement = document.getElementById('fire');
const versionElement = document.getElementById('version');
const ownerElement = document.getElementById('owner');
const expiresElement = document.getElementById('expires');

const lockButton = document.getElementById('lock');
const releaseButton = document.getElementById('release')

const responseElement = document.getElementById('response');

const yawSlider = document.getElementById('yawSlider');
const pitchSlider = document.getElementById('pitchSlider');

const yawDisplay = document.getElementById('yawDisplay');
const pitchDisplay = document.getElementById('pitchDisplay');

const fireButton = document.getElementById('fireButton');

let yaw = 0;
let pitch = 0;
let fire = false;
let version = 0;

let owner = "";
let expires = "";

yawDisplay.innerHTML = yawSlider.value;
pitchDisplay.innerHTML = pitchSlider.value;

async function getState() {
    const url = `${BASEURL}/state`;

    try {
        const response = await fetch(url);
        if (!response.ok) {
            throw new Error(`Error: ${response.status}`);
        }

        const data = await response.json();

        yaw = data.turret.yaw;
        pitch = data.turret.pitch;
        fire = data.turret.fire;
        version = data.turret.version;
        owner = data.lock.owner;
        expires = data.lock.expires;
    } catch (error) {
        console.error("Error:", error.message);
        responseElement.textContent = `Error: ${error.message}`;
    } finally {
        setTimeout(getState, 200);
    }
    yawElement.textContent = `yaw: ${yaw}`;
    pitchElement.textContent = `pitch: ${pitch}`;
    fireElement.textContent = `fire: ${fire}`;
    versionElement.textContent = `version: ${version}`;
    ownerElement.textContent = `owner: ${owner}`;
    expiresElement.textContent = `expires: ${expires}`;
}

async function lock() {
    const url = `${BASEURL}/lock`;

    const request = {
        "user" : `${id}`
    }

    try {
        const response = await fetch(url, {
            method: 'POST',
            headers: {
                contentType: 'application/json'
            },
            body: JSON.stringify(request)
        });
        if (!response.ok) {
            throw new Error(`Error: ${response.status}`);
        }

        const data = await response.json();
        responseElement.textContent = `${JSON.stringify(data)}`;
        console.log(data);
    } catch (error) {
        console.error("Error:", error.message);
        responseElement.textContent = `Error: ${error.message}`;
    }
}

async function release() {
    const url = `${BASEURL}/lock`;

    const request = {
        "user" : `${id}`
    }

    try {
        const response = await fetch(url + `?user=${id}`, {
            method: 'DELETE',
            // headers: {
            //     contentType: 'application/json'
            // },
            // body: JSON.stringify(request)
        });
        if (!response.ok) {
            throw new Error(`Error: ${response.status}`);
        }

        const data = await response.json();
        responseElement.textContent = `${JSON.stringify(data)}`;
        console.log(data);
    } catch (error) {
        console.error("Error:", error.message);
        responseElement.textContent = `Error: ${error.message}`;
    }
}

async function changeState() {
    const url = `${BASEURL}/state`;

    const request = {
        "user" : `${id}`,
        "yaw" : parseInt(yawSlider.value),
        "pitch" : parseInt(pitchSlider.value)
    }

    try {
        const response = await fetch(url, {
            method: 'PUT',
            headers: {
                contentType: 'application/json'
            },
            body: JSON.stringify(request)
        });
        if (!response.ok) {
            throw new Error(`Error: ${response.status}`);
        }
        const data = await response.json();
        responseElement.textContent = `${JSON.stringify(data)}`;
    } catch (error) {
        console.error("Error:", error.message);
        responseElement.textContent = `Error: ${error.message}`;
    }
}

async function fireTurret() {
    const url = `${BASEURL}/fire`;

    const request = {
        "user" : `${id}`
    }

    try {
        const response = await fetch(url, {
            method: 'POST',
            headers: {
                contentType: 'application/json'
            },
            body: JSON.stringify(request)
        });
        if (!response.ok) {
            throw new Error(`Error: ${response.status}`);
        }
        const data = await response.json();
        responseElement.textContent = `${JSON.stringify(data)}`;
    } catch (error) {
        console.error("Error:", error.message);
        responseElement.textContent = `Error: ${error.message}`;
    }
}

yawSlider.oninput = () => {
    yawDisplay.innerHTML = yawSlider.value;
    changeState();
}

pitchSlider.oninput = () => {
    pitchDisplay.innerHTML = pitchSlider.value;
    changeState();
}

lockButton.onclick = () => {
    lock();
}

releaseButton.onclick = () => {
    release();
}

fireButton.onclick = () => {
    if (owner === id) {
        fireTurret();
    } else {
        responseElement.textContent = "You have not got ownership of the lock. Gain ownership before attempting to fire the turret.";
    }
}

getState();