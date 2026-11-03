interface Option {
  name: string;
  blurb?: string;
}

interface Props {
  step: number;
  title: string;
  options: Option[];
  selected: number;
  disabled?: boolean;
  onSelect: (index: number) => void;
}

/** One numbered step. The three of them are the whole interface. */
export function StepPicker({ step, title, options, selected, disabled, onSelect }: Props) {
  return (
    <section className="step">
      <h2>
        <i>Step {step}</i>&nbsp;&nbsp;{title}
      </h2>
      <div className="options">
        {options.map((o, i) => (
          <button
            key={o.name}
            type="button"
            className="option"
            aria-pressed={i === selected}
            disabled={disabled}
            onClick={() => onSelect(i)}
          >
            {o.name}
            {o.blurb ? <small>{o.blurb}</small> : null}
          </button>
        ))}
      </div>
    </section>
  );
}
