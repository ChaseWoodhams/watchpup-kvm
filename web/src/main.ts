import "./style.css";

const app = document.querySelector<HTMLDivElement>("#app");

if (!app) {
  throw new Error("Missing #app root");
}

app.innerHTML = `
  <main class="shell">
    <p class="eyebrow">WatchPup KVM</p>
    <h1>Diagnostics Status</h1>
    <p class="lede">
      This scaffold keeps editable UI source in <code>web/</code> and expects
      firmware to serve <code>/diag</code> JSON for the first milestone.
    </p>
    <section class="panel">
      <h2>First milestone</h2>
      <ul>
        <li>Read-only diagnostics and status page</li>
        <li><code>GET /diag</code> JSON contract</li>
        <li>No HID control in this stage</li>
      </ul>
    </section>
  </main>
`;
