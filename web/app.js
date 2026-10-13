'use strict';
const app = document.getElementById('app');

const session = {
  get token() { try { return sessionStorage.getItem('rtrs.token'); } catch (_) { return null; } },
  set token(v) { try { v ? sessionStorage.setItem('rtrs.token', v) : sessionStorage.removeItem('rtrs.token'); } catch (_) { /* storage blocked */ } },
};

async function api(method, path, body) {
  const headers = { 'Content-Type': 'application/json' };
  if (session.token) headers['Authorization'] = 'Bearer ' + session.token;
  const res = await fetch(path, {
    method,
    headers,
    body: body ? JSON.stringify(body) : undefined,
  });
  let data = {};
  try { data = await res.json(); } catch (_) { /* empty body */ }
  return { ok: res.ok, status: res.status, data };
}

function el(html) {
  const t = document.createElement('template');
  t.innerHTML = html.trim();
  return t.content.firstChild;
}

function showFieldErrors(form, fields) {
  form.querySelectorAll('input').forEach(i => {
    const msg = fields && fields[i.name];
    i.classList.toggle('invalid', !!msg);
    form.querySelector(`[data-err="${i.name}"]`).textContent = msg || '';
  });
}

function registerView() {
  const view = el(`
    <div class="card">
      <h2>Create account</h2>
      <form id="reg" novalidate>
        <label>Full name<input name="name" autocomplete="name"><span class="err" data-err="name"></span></label>
        <label>Email<input name="email" type="email" autocomplete="email"><span class="err" data-err="email"></span></label>
        <label>Phone<input name="phone" inputmode="numeric" maxlength="10" autocomplete="tel"><span class="err" data-err="phone"></span></label>
        <label>Password<input name="password" type="password" autocomplete="new-password"><span class="err" data-err="password"></span></label>
        <button type="submit">Register</button>
        <div id="status"></div>
      </form>
      <p class="muted">Already registered? <a id="to-login">Log in</a></p>
    </div>`);
  view.querySelector('#to-login').addEventListener('click', () => show('login'));
  const form = view.querySelector('form');
  form.addEventListener('submit', async (e) => {
    e.preventDefault();
    const status = form.querySelector('#status');
    const btn = form.querySelector('button');
    btn.disabled = true;
    const payload = Object.fromEntries(new FormData(form));
    const r = await api('POST', '/api/auth/register', payload);
    btn.disabled = false;
    showFieldErrors(form, r.data.fields);
    if (r.ok) {
      form.reset();
      status.innerHTML = '<div class="msg ok">Account created. You can now log in.</div>';
    } else {
      status.innerHTML = '';
      const m = el('<div class="msg bad"></div>');
      m.textContent = r.data.error || 'Registration failed.';
      status.appendChild(m);
    }
  });
  return view;
}

function loginView() {
  const view = el(`
    <div class="card">
      <h2>Log in</h2>
      <form novalidate>
        <label>Email<input name="email" type="email" autocomplete="username"><span class="err" data-err="email"></span></label>
        <label>Password<input name="password" type="password" autocomplete="current-password"><span class="err" data-err="password"></span></label>
        <button type="submit">Log in</button>
        <div id="status"></div>
      </form>
      <p class="muted">New here? <a id="to-register">Create an account</a></p>
    </div>`);
  view.querySelector('#to-register').addEventListener('click', () => show('register'));
  const form = view.querySelector('form');
  form.addEventListener('submit', async (e) => {
    e.preventDefault();
    const btn = form.querySelector('button');
    const status = form.querySelector('#status');
    btn.disabled = true;
    const r = await api('POST', '/api/auth/login', Object.fromEntries(new FormData(form)));
    btn.disabled = false;
    if (r.ok) {
      session.token = r.data.token;
      show('home', r.data.user);
    } else {
      const m = el('<div class="msg bad"></div>');
      m.textContent = r.data.error || 'Login failed.';
      status.replaceChildren(m);
    }
  });
  return view;
}

function homeView(user) {
  const view = el('<div class="card"><h2></h2><p class="muted">You are logged in.</p></div>');
  view.querySelector('h2').textContent = 'Welcome, ' + user.name;
  return view;
}

function show(name, data) {
  const views = { login: loginView, register: registerView, home: () => homeView(data) };
  app.replaceChildren(views[name]());
}
show('login');
